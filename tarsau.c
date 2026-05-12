/*
 * tarsau.c - Basit bir arşivleme aracının uygulaması.
 *
 * İki ana mod desteklenir:
 *   -b : Verilen metin dosyalarını tek bir .sau dosyasında birleştirir.
 *   -a : Verilen .sau dosyasını çözer ve içindeki dosyaları geri oluşturur.
 *
 * Arşiv dosyasının yapısı:
 *   [ 10 byte: organizasyon bölümü toplam boyutu (ASCII, 0-padded) ]
 *   [ |ad,izin,boyut|ad,izin,boyut|...| şeklinde organizasyon gövdesi ]
 *   [ Dosya içerikleri arka arkaya (araya hiçbir şey konmadan) ]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "tarsau.h"

/* Hata mesajlarını ortak şekilde basmak için yardımcı makro */
#define CORRUPT_MSG "Arşiv dosyası uygunsuz veya bozuk!\n"

/* ------------------------------------------------------------------------- */
/* Yardımcı fonksiyonlar                                                      */
/* ------------------------------------------------------------------------- */

int is_text_file(const char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return 0;
    }

    unsigned char buf[4096];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        for (ssize_t i = 0; i < n; i++) {
            if (buf[i] > 127) {
                close(fd);
                return 0;
            }
        }
    }

    close(fd);
    return (n < 0) ? 0 : 1;
}

static const char *base_name(const char *path)
{
    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

/* ------------------------------------------------------------------------- */
/* Arşivleme işlevi (-b)                                                      */
/* ------------------------------------------------------------------------- */

int archive_files(char *files[], int n, const char *output)
{
    FileInfo info[MAX_FILES];
    long total_size = 0;

    if (n <= 0) {
        fprintf(stderr, "En az bir giriş dosyası belirtilmelidir!\n");
        return -1;
    }
    if (n > MAX_FILES) {
        fprintf(stderr, "En fazla %d dosya arşivlenebilir!\n", MAX_FILES);
        return -1;
    }

    for (int i = 0; i < n; i++) {
        struct stat st;

        if (stat(files[i], &st) < 0) {
            fprintf(stderr, "%s dosyası bulunamadı!\n", files[i]);
            return -1;
        }
        if (!S_ISREG(st.st_mode)) {
            fprintf(stderr, "%s normal bir dosya değil!\n", files[i]);
            return -1;
        }
        if (!is_text_file(files[i])) {
            printf("%s giriş dosyasının formatı uyumsuzdur!\n", files[i]);
            return -1;
        }

        strncpy(info[i].name, base_name(files[i]), MAX_NAME - 1);
        info[i].name[MAX_NAME - 1] = '\0';
        info[i].perm = st.st_mode & 0777;
        info[i].size = (long)st.st_size;

        total_size += info[i].size;
        if (total_size > MAX_TOTAL_SIZE) {
            fprintf(stderr, "Toplam dosya boyutu 200 MB'yi aşamaz!\n");
            return -1;
        }
    }

    char org_body[12288];
    int  pos = 0;

    pos += snprintf(org_body + pos, sizeof(org_body) - pos, "|");
    for (int i = 0; i < n; i++) {
        int written = snprintf(org_body + pos, sizeof(org_body) - pos,
                               "%s,%o,%ld|",
                               info[i].name, info[i].perm, info[i].size);
        if (written < 0 || (size_t)written >= sizeof(org_body) - pos) {
            fprintf(stderr, "Dosya adları çok uzun, tampon yetmedi!\n");
            return -1;
        }
        pos += written;
    }

    int org_total = ORG_HEADER_SIZE + pos;

    int out = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out < 0) {
        fprintf(stderr, "Çıkış dosyası oluşturulamadı: %s\n", strerror(errno));
        return -1;
    }

    char header[ORG_HEADER_SIZE + 1];
    snprintf(header, sizeof(header), "%010d", org_total);
    if (write(out, header, ORG_HEADER_SIZE) != (ssize_t)ORG_HEADER_SIZE) {
        fprintf(stderr, "Header yazılamadı!\n");
        close(out);
        return -1;
    }

    if (write(out, org_body, pos) != (ssize_t)pos) {
        fprintf(stderr, "Organizasyon bölümü yazılamadı!\n");
        close(out);
        return -1;
    }

    char buf[8192];
    for (int i = 0; i < n; i++) {
        int in = open(files[i], O_RDONLY);
        if (in < 0) {
            fprintf(stderr, "%s açılamadı!\n", files[i]);
            close(out);
            return -1;
        }

        ssize_t r;
        while ((r = read(in, buf, sizeof(buf))) > 0) {
            if (write(out, buf, r) != r) {
                fprintf(stderr, "Yazma hatası!\n");
                close(in);
                close(out);
                return -1;
            }
        }
        if (r < 0) {
            fprintf(stderr, "Okuma hatası: %s\n", strerror(errno));
            close(in);
            close(out);
            return -1;
        }
        close(in);
    }

    close(out);
    printf("Dosyalar birleştirildi.\n");
    return 0;
}

/* ------------------------------------------------------------------------- */
/* Çıkartma işlevi (-a)                                                       */
/* ------------------------------------------------------------------------- */

static int parse_org(char *body, FileInfo info[])
{
    int   count = 0;
    char *p     = body;

    while (count < MAX_FILES) {
        if (*p != '|') break;
        p++;
        if (*p == '\0') break;

        char *comma1 = strchr(p, ',');
        if (!comma1) return -1;
        int name_len = comma1 - p;
        if (name_len <= 0 || name_len >= MAX_NAME) return -1;
        memcpy(info[count].name, p, name_len);
        info[count].name[name_len] = '\0';
        p = comma1 + 1;

        char *comma2 = strchr(p, ',');
        if (!comma2) return -1;
        int perm_len = comma2 - p;
        if (perm_len <= 0 || perm_len >= 16) return -1;
        char perm_buf[16];
        memcpy(perm_buf, p, perm_len);
        perm_buf[perm_len] = '\0';
        info[count].perm = (mode_t)strtol(perm_buf, NULL, 8);
        p = comma2 + 1;

        char *pipe = strchr(p, '|');
        if (!pipe) return -1;
        int size_len = pipe - p;
        if (size_len <= 0 || size_len >= 32) return -1;
        char size_buf[32];
        memcpy(size_buf, p, size_len);
        size_buf[size_len] = '\0';
        info[count].size = atol(size_buf);
        if (info[count].size < 0) return -1;
        p = pipe;

        count++;
    }

    return count;
}

int extract_archive(const char *archive, const char *dir)
{
    int fd = open(archive, O_RDONLY);
    if (fd < 0) {
        printf(CORRUPT_MSG);
        return -1;
    }

    char header[ORG_HEADER_SIZE + 1];
    if (read(fd, header, ORG_HEADER_SIZE) != (ssize_t)ORG_HEADER_SIZE) {
        printf(CORRUPT_MSG);
        close(fd);
        return -1;
    }
    header[ORG_HEADER_SIZE] = '\0';

    for (int i = 0; i < ORG_HEADER_SIZE; i++) {
        if (header[i] < '0' || header[i] > '9') {
            printf(CORRUPT_MSG);
            close(fd);
            return -1;
        }
    }

    int org_total = atoi(header);
    if (org_total <= ORG_HEADER_SIZE) {
        printf(CORRUPT_MSG);
        close(fd);
        return -1;
    }
    int body_len = org_total - ORG_HEADER_SIZE;

    char *body = (char *)malloc(body_len + 1);
    if (!body) {
        fprintf(stderr, "Bellek yetersiz!\n");
        close(fd);
        return -1;
    }
    if (read(fd, body, body_len) != (ssize_t)body_len) {
        printf(CORRUPT_MSG);
        free(body);
        close(fd);
        return -1;
    }
    body[body_len] = '\0';

    FileInfo info[MAX_FILES];
    int count = parse_org(body, info);
    free(body);

    if (count <= 0) {
        printf(CORRUPT_MSG);
        close(fd);
        return -1;
    }

    if (dir) {
        struct stat st;
        if (stat(dir, &st) < 0) {
            if (mkdir(dir, 0755) < 0) {
                fprintf(stderr, "%s dizini oluşturulamadı: %s\n",
                        dir, strerror(errno));
                close(fd);
                return -1;
            }
        } else if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "%s zaten var ve dizin değil!\n", dir);
            close(fd);
            return -1;
        }
    }

    char buf[8192];
    for (int i = 0; i < count; i++) {
        char path[MAX_PATH];
        if (dir) {
            snprintf(path, sizeof(path), "%s/%s", dir, info[i].name);
        } else {
            snprintf(path, sizeof(path), "%s", info[i].name);
        }

        int out = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out < 0) {
            fprintf(stderr, "%s oluşturulamadı!\n", path);
            close(fd);
            return -1;
        }

        long remaining = info[i].size;
        while (remaining > 0) {
            ssize_t to_read = (remaining > (long)sizeof(buf))
                                ? (ssize_t)sizeof(buf)
                                : (ssize_t)remaining;
            ssize_t got = read(fd, buf, to_read);
            if (got <= 0) {
                printf(CORRUPT_MSG);
                close(out);
                close(fd);
                return -1;
            }
            if (write(out, buf, got) != got) {
                fprintf(stderr, "Yazma hatası!\n");
                close(out);
                close(fd);
                return -1;
            }
            remaining -= got;
        }
        close(out);

        if (chmod(path, info[i].perm) < 0) {
            fprintf(stderr, "%s için izinler ayarlanamadı.\n", path);
        }
    }

    close(fd);

    printf("%s dizininde ", dir ? dir : "mevcut");
    for (int i = 0; i < count; i++) {
        printf("%s%s", info[i].name, (i < count - 1) ? ", " : "");
    }
    printf(" dosyaları açıldı.\n");

    return 0;
}

/* ------------------------------------------------------------------------- */
/* Kullanım yardımı ve main                                                   */
/* ------------------------------------------------------------------------- */

static void print_usage(void)
{
    printf("Kullanım:\n");
    printf("  Arşivleme  : ./tarsau -b dosya1 dosya2 ... [-o cikis.sau]\n");
    printf("  Çıkartma   : ./tarsau -a arsiv.sau [hedef_dizin]\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage();
        return 1;
    }

    if (strcmp(argv[1], "-b") == 0) {
        char       *input_files[MAX_FILES];
        int         n      = 0;
        const char *output = DEFAULT_OUTPUT;

        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0) {
                if (i + 1 >= argc) {
                    fprintf(stderr, "-o sonrası çıkış dosyası belirtilmedi!\n");
                    return 1;
                }
                output = argv[++i];
            } else {
                if (n >= MAX_FILES) {
                    fprintf(stderr,
                            "En fazla %d dosya arşivlenebilir!\n", MAX_FILES);
                    return 1;
                }
                input_files[n++] = argv[i];
            }
        }

        return (archive_files(input_files, n, output) == 0) ? 0 : 1;
    }

    if (strcmp(argv[1], "-a") == 0) {
        if (argc < 3 || argc > 4) {
            print_usage();
            return 1;
        }
        const char *archive = argv[2];
        const char *dir     = (argc == 4) ? argv[3] : NULL;
        return (extract_archive(archive, dir) == 0) ? 0 : 1;
    }

    print_usage();
    return 1;
}
