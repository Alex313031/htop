/*
htop - History.c
(C) 2004-2026 htop dev team
Released under the GNU GPLv2+, see the COPYING file
in the source distribution for its full text.
*/

#include "config.h" // IWYU pragma: keep

#include "History.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "Macros.h"
#include "XUtils.h"


/* Determine whether the history file is safe to (over)write, mirroring the
   checks Settings_read() applies to htoprc: the file must be a regular file
   owned by the effective user with owner-write permission. The O_NOFOLLOW
   flag guards the final path component against symlink attacks, while
   O_NONBLOCK keeps an owned FIFO from blocking the read-only fallback. */
static void History_load(History* this) {
   if (!this->filename)
      return;

   int fd = -1;
   do {
      fd = open(this->filename, O_RDWR | O_NOCTTY | O_NOFOLLOW | O_NONBLOCK);
   } while (fd < 0 && errno == EINTR);

   if (fd < 0) {
      this->writeHistory = (errno == ENOENT);
      if (errno != EACCES && errno != EPERM && errno != EROFS)
         return;
   } else {
      struct stat sb;
      int err = fstat(fd, &sb);
      this->writeHistory = !err && S_ISREG(sb.st_mode) && (sb.st_mode & S_IWUSR) && sb.st_uid == geteuid();
   }

   /* If opening read & write is not possible, open read only.
      O_NOFOLLOW rejects a planted symlink, O_NONBLOCK avoids blocking on
      non-regular files such as FIFOs when no writer is present. */
   if (fd < 0) {
      do {
         fd = open(this->filename, O_RDONLY | O_NOCTTY | O_NOFOLLOW | O_NONBLOCK);
      } while (fd < 0 && errno == EINTR);
   }

   if (fd < 0)
      return;

   /* Only read regular files; reading a FIFO would block. */
   struct stat sb;
   if (fstat(fd, &sb) != 0 || !S_ISREG(sb.st_mode)) {
      close(fd);
      return;
   }

   FILE* fp = fdopen(fd, "r");
   if (!fp) {
      close(fd);
      return;
   }

   char line[LINEEDITOR_MAX + 2];
   while (fgets(line, sizeof(line), fp)) {
      size_t len = strlen(line);
      /* strip trailing newline */
      while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
         line[--len] = '\0';
      if (len == 0)
         continue;

      History_add(this, line);
   }
   fclose(fp);
}

History* History_new(const char* filename) {
   History* this = xCalloc(1, sizeof(History));
   this->capacity = 64;
   this->entries = xCalloc(this->capacity, sizeof(char*));
   this->count = 0;
   this->position = 0;
   this->saved[0] = '\0';
   this->filename = filename ? xStrdup(filename) : NULL;
   this->writeHistory = true;

   if (this->filename)
      History_load(this);

   this->position = this->count;

   return this;
}

void History_delete(History* this) {
   for (size_t i = 0; i < this->count; i++)
      free(this->entries[i]);
   free(this->entries);
   free(this->filename);
   free(this);
}

void History_save(const History* this) {
   if (!this->filename || !this->writeHistory)
      return;
   /* Settings_write writes things via a temp file & rename, we do it less robust but faster here.
      O_NOFOLLOW guards against a symlink planted at the final path component,
      O_NONBLOCK avoids hanging on an existing FIFO, and the fstat() re-check
      closes a race between open and the owner verification. */
   int fd = open(this->filename, O_WRONLY | O_NOCTTY | O_CREAT | O_NOFOLLOW | O_NONBLOCK, 0600);
   if (fd == -1)
      return;

   struct stat sb;
   if (fstat(fd, &sb) != 0 || !S_ISREG(sb.st_mode) || !(sb.st_mode & S_IWUSR) || sb.st_uid != geteuid()) {
      close(fd);
      return;
   }

   if (ftruncate(fd, 0) != 0) {
      close(fd);
      return;
   }

   FILE* fp = fdopen(fd, "w");
   if (!fp) {
      close(fd); // fd not consumed on failure, so close it
      return;
   }
   size_t start = (this->count > HISTORY_MAX_ENTRIES) ? this->count - HISTORY_MAX_ENTRIES : 0;
   for (size_t i = start; i < this->count; i++)
      fprintf(fp, "%s\n", this->entries[i]);
   fclose(fp);
}

void History_add(History* this, const char* entry) {
   if (!entry || entry[0] == '\0')
      return;

   /* Deduplicate: remove previous identical entry if present */
   for (size_t i = 0; i < this->count; i++) {
      if (String_eq(this->entries[i], entry)) {
         free(this->entries[i]);
         memmove(this->entries + i, this->entries + i + 1, (this->count - i - 1) * sizeof(char*));
         this->count--;
         break;
      }
   }

   /* Grow array if needed */
   if (this->count >= this->capacity) {
      if (this->capacity < HISTORY_MAX_ENTRIES) {
         this->capacity = MINIMUM(this->capacity * 2, (size_t)HISTORY_MAX_ENTRIES);
         this->entries = xReallocArray(this->entries, this->capacity, sizeof(char*));
      } else {
         /* Drop oldest entry */
         free(this->entries[0]);
         memmove(this->entries, this->entries + 1, (this->count - 1) * sizeof(char*));
         this->count--;
      }
   }

   this->entries[this->count++] = xStrdup(entry);

   /* Reset position to "at new input" */
   this->position = this->count;
   this->saved[0] = '\0';
}

const char* History_navigate(History* this, LineEditor* editor, bool back) {
   if (this->count == 0)
      return NULL;

   if (back) {
      /* Going back (up arrow) */
      if (this->position == this->count) {
         /* Save current editor content before entering history */
         strncpy(this->saved, LineEditor_getText(editor), LINEEDITOR_MAX);
         this->saved[LINEEDITOR_MAX] = '\0';
      }
      if (this->position > 0) {
         this->position--;
         return this->entries[this->position];
      }
      return NULL; /* Already at oldest entry */
   } else {
      /* Going forward (down arrow) */
      if (this->position >= this->count)
         return NULL; /* Already at newest */
      this->position++;
      if (this->position == this->count) {
         /* Restore saved input */
         return this->saved;
      }
      return this->entries[this->position];
   }
}

void History_resetPosition(History* this) {
   this->position = this->count;
   this->saved[0] = '\0';
}
