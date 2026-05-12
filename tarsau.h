#ifndef TARSAU_H
#define TARSAU_H

#include <sys/types.h>

/* Proje sabitleri */
#define MAX_FILES        32                       /* En fazla arsivlenebilir dosya sayisi */
#define MAX_TOTAL_SIZE   (200L * 1024L * 1024L)   /* Toplam giris boyutu siniri: 200 MB */
#define MAX_NAME         256                      /* Dosya adi icin maksimum uzunluk */
#define MAX_PATH         1024                     /* Tam yol icin tampon uzunlugu */
#define ORG_HEADER_SIZE  10                       /* Organizasyon bolumunun ilk 10 byte'i */
#define DEFAULT_OUTPUT   "a.sau"                  /* -o verilmediginde varsayilan cikis */

/* Arsivde tutulan tek bir dosyanin metadata'sini temsil eder. */
typedef struct {
    char    name[MAX_NAME]; /* Yalnizca dosya adi (path bilesenleri ayiklanmis) */
    mode_t  perm;           /* Octal izinler (st_mode & 0777) */
    long    size;           /* Byte cinsinden dosya boyutu */
} FileInfo;

/* Genel fonksiyon prototipleri */
int  archive_files(char *files[], int n, const char *output);
int  extract_archive(const char *archive, const char *dir);
int  is_text_file(const char *path);

#endif /* TARSAU_H */
