#include <../include/entradasalida.h>

void crear_bitmap(){
    int tam_bitmap = ceil(block_count / 8.0);
    int bitmap_fd = open(path_bitmap, O_CREAT | O_RDWR, 0744);

    if (bitmap_fd == -1) {
        log_error(logger_entradasalida, "Error al crear el archivo de bitmap");
        exit(EXIT_FAILURE);
    }

    struct stat st;
    if (fstat(bitmap_fd, &st) == -1) {
        log_error(logger_entradasalida, "Error al obtener el tamaño del archivo de bitmap");
        close(bitmap_fd);
        exit(EXIT_FAILURE);
    }
    off_t current_size = st.st_size;

    if (current_size < tam_bitmap) {
        if (ftruncate(bitmap_fd, tam_bitmap) == -1) { 
            log_error(logger_entradasalida, "Error al ajustar el tamaño del archivo");
            close(bitmap_fd);
            exit(EXIT_FAILURE);
        }
    }

    void* mapeo_bitmap = mmap(NULL, tam_bitmap, PROT_READ | PROT_WRITE, MAP_SHARED, bitmap_fd, 0);
    if (mapeo_bitmap == MAP_FAILED) {
        log_error(logger_entradasalida, "Error al mapear el archivo de bitmap");
        close(bitmap_fd);
        exit(EXIT_FAILURE);
    }

    bitmap = bitarray_create_with_mode(mapeo_bitmap, tam_bitmap, LSB_FIRST);
    
    if (lseek(bitmap_fd, 0, SEEK_END) == 0) { // Si el archivo estaba vacío
        for(int i = 0; i < bitarray_get_max_bit(bitmap); i++) {
            bitarray_clean_bit(bitmap, i); // Establecer todos los bits en 0
        }
    }

    if (msync(mapeo_bitmap, tam_bitmap, MS_SYNC) == -1) {
        log_error(logger_entradasalida, "Error al sincronizar el bitmap con el archivo");
        exit(EXIT_FAILURE);
    }
    
    close(bitmap_fd);
}
