# tarsau - Arşivleme Aracı

## Proje Hakkında
Bu proje, Sakarya Üniversitesi Bilgisayar Mühendisliği Sistem Programlama dersi kapsamında geliştirilmiş bir komut satırı arşivleme aracıdır. [cite_start]`tar`, `rar` veya `zip` gibi çalışır ancak **sıkıştırma (compression) yapmaz**[cite: 11, 239]. [cite_start]Birden fazla metin (ASCII) dosyasını tek bir `.sau` uzantılı arşiv dosyasında birleştirir ve istendiğinde orijinal dosya izinlerini (okuma, yazma, çalıştırma) koruyarak geri çıkarır[cite: 14, 18, 266].

## Özellikler ve Sınırlar
* [cite_start]**Sadece Metin Dosyaları:** Program yalnızca ASCII formatındaki metin dosyalarını kabul eder (Karakter başına 1 bayt)[cite: 16, 248]. [cite_start]Uyumsuz dosyalarda işlem durdurulur[cite: 256].
* [cite_start]**Kapasite:** En fazla 32 adet dosya arşivlenebilir[cite: 14, 255].
* [cite_start]**Boyut Sınırı:** Giriş dosyalarının toplam boyutu 200 MB'ı geçemez[cite: 14, 254].
* **POSIX Standartları:** Geliştirme sürecinde alt seviye POSIX dosya sistemi çağrıları (`open`, `read`, `write`, `chmod`, `mkdir` vb.) kullanılmıştır.

## Kurulum ve Derleme
[cite_start]Proje `make` aracı ile Linux/Unix ortamlarında kolayca derlenebilir[cite: 25, 278]. Terminalde proje dizinine giderek şu komutları çalıştırabilirsiniz:

```bash
# Projeyi derlemek için:
make

# Üretilen derlenmiş dosyaları (.o ve çalıştırılabilir dosya) temizlemek için:
make clean
