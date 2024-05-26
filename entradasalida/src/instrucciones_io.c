#include "../include/instrucciones_io.h"

void io_gen_sleep(int unidad_trabajo, int tiempo_unidad_trabajo) {
    sleep(unidad_trabajo * tiempo_unidad_trabajo);
}