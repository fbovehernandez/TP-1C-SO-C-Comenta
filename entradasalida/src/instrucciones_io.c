#include "../include/instrucciones_io.h"

// NO esta siendo usado
void io_gen_sleep_func(int unidad_trabajo, int tiempo_unidad_trabajo) {
    sleep(unidad_trabajo * tiempo_unidad_trabajo);
}