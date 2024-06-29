#include <../include/entradasalida.h>

uint32_t buscar_bloque_libre()
{

    uint32_t bloque_libre = -1;

    uint32_t tamanio_bitmap = bitarray_get_max_bit(bitmap);
    for (int i = 0; i < tamanio_bitmap; i++)
    {

        if (!bitarray_test_bit(bitmap, i))
        { // el bit en la pos i del bitmap es 0 osea esta libre
            bloque_libre = i;
            break;
        }
    }
    return bloque_libre;
}

void asignar_bloque(uint32_t bloque_libre) {
    if (bloque_libre >= bitarray_get_max_bit(bitmap)) {
        log_error(logger_entradasalida, "Bloque %d fuera de rango", bloque_libre);
        return;
    }

    if (bitarray_test_bit(bitmap, bloque_libre)) {
        log_error(logger_entradasalida, "Bloque %d ya está ocupado", bloque_libre);
        return;
    }

    // Marcar el bloque como ocupado en el bitmap
    bitarray_set_bit(bitmap, bloque_libre);

    log_info(logger_entradasalida, "Bloque %d asignado al archivo %s", bloque_libre);
}
