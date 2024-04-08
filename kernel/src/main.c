#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>

int main(int argc, char* argv[]) {
    int respuesta;
    do {
        printf("---------------Consola interactiva Kernel---------------\n");
        printf("Elija una opcion (numero)\n");
        printf("1- Ejecutar Script \n");
        printf("2- Iniciar proceso\n");
        printf("3- Finalizar proceso\n");
        printf("4- Iniciar planificacion\n");
        printf("5- Detener planificacion\n");
        printf("6- Listar procesos por estado\n");
        printf("7- Finalizar modulo");
        scanf("%d",respuesta);
        switch (respuesta)
        {
        case 1:
            EJECUTAR_SCRIPT(/* pathing del conjunto de instrucciones*/);
            break;
        
        case 2:
            INICIAR_PROCESO(/*Recibe el pathing*/);
            break;
        
        case 3:
            FINALIZAR_PROCESO(/*pid*/);
            break;
        
        case 4:
            INICIAR_PLANIFICACION(); // Esto depende del algoritmo vigente (FIFO, RR,VRR), se deben poder cambiar
            break;
        
        case 5:
            DETENER_PLANIFICACION(); // Ver linea 33
            break;
        
        case 6:
            PROCESO_ESTADO();
            break;
        
        case 7:
            printf("Finalizacion del modulo");
            exit(1); 
            break;
        
        default:
            printf("No se encuentra el numero dentro de las opciones, porfavor elija una correcta");
            break;
        }
        
    } while(respuesta != 7); // Modificar respuesta si se agregan mas elementos en la consola
    return 0;
}

// La idea es que el usuario (nosotros) no estemos todo el dia con la consola interactiva
// Por eso tenemos el ejecutar escript, que recibe una lista de instrucciones que debe realizar en seguidilla este modulo
void EJECUTAR_SCRIPT(/*path*/)
{

}

void INICIAR_PROCESO(/*path*/)
{

}

void FINALIZAR_PROCESO(/*pid*/)
{

}

void DETENER_PLANIFICACION()
{

}

void INICIAR_PLANIFICACION()
{

}

void PROCESO_ESTADO() // Esta funcion es para listar los procesos estado
{

}