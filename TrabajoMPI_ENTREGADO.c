#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"


//Crearemos posteriormente un vector de este struct con la información correspondiente para cada proceso buscador
struct Proceso {
    long int sumaParcialTotal;
    long int sumaDivisores;
    int contNumDivisores;
   	float tiempoEjecucion;
};



void main (int argc, char** argv){

	MPI_Status status,status2, statusAcumCompartido;
	MPI_Request peticionTerminado,peticionEncontrado,peticionAcumCompartido;
	int id;
	int nprc;
	double tInit,tFin,tTotal;
	double tInitTotal;
	float tFinalTotal=0.0;
	char nomProcesador [MPI_MAX_PROCESSOR_NAME];
	int longitudNombre;
	int etiq1=12,etiq2=50,divisorEncontrado=8,procesoTerminado=10,acumCompartido=24,envioSumaTotal=14;
	long int valor, topeMin, topeMax, msjInt,flagCincoPorcent,cincoPorcent=0;
	long int nNumeros, nNumerosUlt,lnprc;
	int nTerminados=0;
	unsigned long msjIntRcv;
	double vectMsjInt[2];
	int sumar;
	long int acum=0, divSum=0, divAcumCompartido=0, divAcumCompartidoRecepcion=0,sumTotalUltProc=0,acumSumaTotal=0; 
	int  contMensajesRecvOtroProc=0, contTotalDivisores=0;
	int j;
	int flagMod=0,flag=0,flagSumTotalUltProc=0,flagCompartido=0,flagPrimIsnd=1;

	//VARIABLES PARA PRUEBAS
	int encontrado;


	MPI_Init(&argc,&argv);
	

	MPI_Comm_rank(MPI_COMM_WORLD, &id);
	MPI_Comm_size(MPI_COMM_WORLD, &nprc);
	MPI_Get_processor_name(nomProcesador,&longitudNombre);



	//SOLO PARA EL PROCESO 0 CONOCERÁ INICIALMENTE EL VALOR Y SE LO TRANSMITIRÁ AL RESTO
	if(id==0)
	{
		if(argc<2)
		{
			printf("(%d) Indica el numéro que quieres comprobar: \n" ,id);
			scanf("%ld",&valor);
		}
		else if(argc==2)
		{
			if(nprc<=1)
			{
				printf("\n(%d) EL NUMERO DE PROCESOS INTRODUCIDOS NO ES CORRECTO DEBE HABER AL MENOS 2 PROCESOS (UNO RAIZ Y UNO BUSCADOR)\n\n",id);
				exit(0);	
			}

			valor=atol(argv[1]);
			printf("EL VALOR INTRODUCIDO ES %ld\n",valor);
		}
		else
		{
			printf("\n(%d) HAS INTRODUCIDO DEMASIADOS ARGUMENTOS, VUELVE A EJECUTAR EL PROGRAMA\n\n",id);
			exit(0);
		}


			lnprc=(long int)(nprc-1);
			nNumeros=valor/lnprc;

	}

	MPI_Bcast(&valor,1, MPI_LONG,0,MPI_COMM_WORLD);

	














	//AQUÍ ENTRARÁ CUALQUIER PROCESO QUE NO SEA EL QUE TIENE ID=0, ES DECIR, LOS QUE BUSCARÁN LOS DIVISORES
	if(id!=0)
	{
			
		if(id<nprc-1)
		{
			MPI_Recv(&msjInt,1,MPI_LONG,0,etiq1, MPI_COMM_WORLD,&status);
			topeMax=msjInt*id;
			topeMin=topeMax-msjInt+1;

			//Esta variable nos permitirá hacer un mpiTest, tan solo un 5% de las veces totales de esta forma liberaremos gran cantidad de carga al procesador
			cincoPorcent=(topeMax-topeMin)*5/100;
		}
		//Aquí entrará el último proceso que es el único que recibe un vector de enteros en lugar de un entero
		else
		{	
			MPI_Recv(vectMsjInt,2,MPI_DOUBLE,0,etiq1, MPI_COMM_WORLD,&status);
			topeMin=vectMsjInt[0]*(id-1) +1;
			topeMax=topeMin-1+vectMsjInt[1];

			//Esta variable nos permitirá hacer un mpiTest, tan solo un 5% de las iteraciones totales de búsqueda de duvisor, de esta forma liberaremos gran cantidad de carga al procesador
			cincoPorcent=(topeMax-topeMin)*5/100;
		}



		acum=0;
		divSum=0;

		//Hacemos la petición de recepción por si otro proceso menor nos envía sun info porque otro anterior haya terminado
		MPI_Irecv(&divAcumCompartidoRecepcion,1,MPI_LONG,id-1,acumCompartido, MPI_COMM_WORLD,&peticionAcumCompartido);


		//VALOR INICIAL DE TIEMPO
		tInit=MPI_Wtime();

		//BUCLE ENCARGADO DE OBTENER LOS DIVISORES
		for(long int i=topeMin; i<=topeMax; i++)
		{
			if(valor%i==0)
			{
				acum+=1;
				divSum+=i;
				divAcumCompartido+=i;

				MPI_Send(&i,1,MPI_LONG,0,divisorEncontrado,MPI_COMM_WORLD);
			}

			//El valor tope no tiene que ser probado
			if(i==topeMax-1 && id==nprc-1)
				break;

			flagCincoPorcent++;
			//Comprobamos si hemos recibido un mensaje del proceso x-1
			if(flagCincoPorcent>=cincoPorcent)
			{
				MPI_Test(&peticionAcumCompartido,&flagCompartido,&statusAcumCompartido);
				flagCincoPorcent=0;
			}

			//SI HEMOS RECIBIDO UN MENSAJE Y EL CONTADOR DE MENSAJES QUE TENEMOS POR RECIBIR AUN INDICA QUE DEBEMOS RECIBIR MAS MENSAJES ACTUAMOS
			if(flagCompartido && contMensajesRecvOtroProc!=id-1)
			{
				flagCompartido=0; 

				if(id==nprc-1)
					sumTotalUltProc+=divAcumCompartidoRecepcion;

				//RECOGEMOS EL SUMADOR PARCIAL DEL POCESO ID-1, SE LO AÑADIMOS AL SUMADOR PARCIAL DEL PROCESO QUE LO HA RECOGIDO Y SE LO ENVIAMOS AL PROCESO ID+1 PARA QUE HAGA LO MISMO
				if(id<nprc-1)
				{
					divAcumCompartido+=divAcumCompartidoRecepcion;
					MPI_Send(&divAcumCompartido,1,MPI_LONG,id+1,acumCompartido,MPI_COMM_WORLD);
					divAcumCompartido=0;
				}
				contMensajesRecvOtroProc++;
				//COMO SABEMOS QUE YA HEMOS RECIBIDO UN MENSAJE, HACEMOS OTRA PETCIÓN DE RECEPCIÓN
				MPI_Irecv(&divAcumCompartidoRecepcion,1,MPI_LONG,id-1,acumCompartido, MPI_COMM_WORLD,&peticionAcumCompartido);

			}


		}

		//TIEMPO FINAL
		tFin=MPI_Wtime();
		tTotal=tFin-tInit;		


		//SI AUN NO HEMOS RECIBIDO ALGUNO O NINGUNO DE LOS MENSAJES DE LOS OTROS PROCESOS, ESPERAMOS A RECIBIRLOS (POR EJEMPLO SI SOY EL PROCESO 3, TENGO QUE HABER RECIBIDO 2 MENSAJES)
		while(contMensajesRecvOtroProc!=id-1)
		{
			//Usamos un wait en lugar de test ya que de esta forma el proceso en cuestión se quedará bloqueado sin consumir CPU esperando a recibir el mensaje
			MPI_Wait(&peticionAcumCompartido,&statusAcumCompartido);
			
				if(id==nprc-1)
					sumTotalUltProc+=divAcumCompartidoRecepcion;

				//RECOGEMOS EL SUMADOR PARCIAL DEL POCESO ID-1, SE LO AÑADIMOS AL SUMADOR PARCIAL DEL PROCESO QUE LO HA RECOGIDO Y SE LO ENVIAMOS AL PROCESO ID+1 PARA QUE HAGA LO MISMO
				if(id<nprc-1)
				{
					divAcumCompartido+=divAcumCompartidoRecepcion;
					MPI_Send(&divAcumCompartido,1,MPI_LONG,id+1,acumCompartido,MPI_COMM_WORLD);
					divAcumCompartido=0;
				}
				contMensajesRecvOtroProc++;

				//SI AUN NOS QUEDAN MÁS ITERACIONES HACEMOS OTRA PETCIÓN DE RECEPCIÓN
				if(contMensajesRecvOtroProc!=id-1)
					MPI_Irecv(&divAcumCompartidoRecepcion,1,MPI_LONG,id-1,acumCompartido, MPI_COMM_WORLD,&peticionAcumCompartido);	
		}


		//Cada proceso le enviará su suma parcial al proceso 0 además de su tiempo de ejecución, se enviará un vector double de dimensión 2
		//con ls suma parcial que ha calculado entre todos sus divisores y el tiempo que ha tardado en encontrarlos todos.
		vectMsjInt[0]=divSum;
		vectMsjInt[1]=tTotal;
		//printf("(%d) He encontrado %ld divisores y mi suma parcial es de %ld\n\n" ,id,acum,divSum);
		MPI_Send(&vectMsjInt,2,MPI_DOUBLE,0,procesoTerminado,MPI_COMM_WORLD);

		//Ahora se lo enviaremos también al proceso id+1 en caso de que no seamos el útlimo proceso
		if(id<nprc-1)
		{
			MPI_Send(&divAcumCompartido,1,MPI_LONG,id+1,acumCompartido,MPI_COMM_WORLD);
		}

		//Ahora se lo enviaremos también al proceso 0 en caso de que seamos el útlimo proceso
		if(id==nprc-1)
		{
			sumTotalUltProc+=divSum;
			//printf("(%d) El sumatorio compartido de todos los procesos tiene un total de %ld\n",id,sumTotalUltProc);
			MPI_Send(&sumTotalUltProc,1,MPI_LONG,0,envioSumaTotal,MPI_COMM_WORLD);
		}

	}















	//AQUÍ ENTRARÁ EL PROCESO 0, ENCARGADO DE SUMAR Y DE RECOGER LOS DIVISORES  QUE EL RESTO LE ENVÍEN  
	if(id==0)
	{
		struct Proceso procesos[nprc-1];

		//Inicializamos las variables del struct
		for(int i=0; i<nprc-1;i++)
		{
			procesos[i].sumaParcialTotal=0;
			procesos[i].sumaDivisores=0;
			procesos[i].contNumDivisores=0;
			procesos[i].tiempoEjecucion=0.0;
		}

		
	
		//INICIAMOS LA VARIABLE DE TIEMPO DE PROCESAMIENTO PARA EL PROCESO 0
		tInitTotal=MPI_Wtime();

		//Si el módulo del numero de procesos y el valor es distinto de 0 quiere decir que va a haber un proceso
		//en este caso será el último, que se encargará de hacer el mismo número de números que el resto mas 
		//los que quedaban por hacer
		if((valor%(nprc-1))!=0)
		{
			nNumerosUlt=valor-(nprc-2)*nNumeros;
			vectMsjInt[0]=nNumeros;
			vectMsjInt[1]=nNumerosUlt;
		}

		//En caso de que el módulo del valor y el numero de procesos sea 0, quiere decir que podemos dar a todos los procesos
		//el mísmo numero de números para trabajar.
		else
		{
			vectMsjInt[0]=nNumeros;
			vectMsjInt[1]=nNumeros;
		}


		//Enviaremos a cada proceso el número de números con los que tiene que trabajar, y al último de los procesos, 
		//por si el módulo hubiese dado distinto de 0, le enviaremos un vector con el número de numeros que tiene que
		//trabajar y el número tope hasta el que tiene que llegar
		for(int i=1; i<nprc-1; i++)
		{
			MPI_Send(&nNumeros,1,MPI_LONG,i,etiq1,MPI_COMM_WORLD);
		}
			MPI_Send(vectMsjInt,2,MPI_DOUBLE,nprc-1,etiq1,MPI_COMM_WORLD);


		//AQUÍ SE HABRÁN REPARTIDO LOS RANGOS DE NÚMEROS PARA DIVIDIR ENTRE CADA PROCESO

		while(nTerminados<nprc-1 || flag)
		{

			MPI_Probe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&status);

				//COMPROBAMOS SI ES UN MENSAJE DE PROCESO TERMINADO YA QUE EN ESTE CASO TENEMOS QUE RECIBIR UN ARRAY CON EL SUMADOR TOTAL Y EL TIEMPO TOTAL
				if(status.MPI_TAG==procesoTerminado)
				{
					MPI_Recv(&vectMsjInt,2,MPI_DOUBLE,status.MPI_SOURCE,status.MPI_TAG, MPI_COMM_WORLD,&status2);
				}

				else
				{
					MPI_Recv(&acum,1,MPI_LONG,status.MPI_SOURCE,status.MPI_TAG, MPI_COMM_WORLD,&status2);
				}


				if(status.MPI_TAG==divisorEncontrado)
				{
					procesos[status.MPI_SOURCE-1].sumaDivisores+=acum;
					procesos[status.MPI_SOURCE-1].contNumDivisores++;
					acumSumaTotal+=acum;

					printf("DIV ENC: \t P(%d) \t DIV(%ld) \t NDIVS(%d) \t SUMDIVS(%ld)\n",status.MPI_SOURCE,acum,procesos[status.MPI_SOURCE-1].contNumDivisores,(long int)procesos[status.MPI_SOURCE-1].sumaDivisores);
				}

				if(status.MPI_TAG==procesoTerminado)
				{
					procesos[status.MPI_SOURCE-1].sumaParcialTotal=vectMsjInt[0];
					procesos[status.MPI_SOURCE-1].tiempoEjecucion=vectMsjInt[1];
					nTerminados++;

					printf("FIN PRO: \t P(%d) \t SUM PARC(%ld)\n",status.MPI_SOURCE,(long int)vectMsjInt[0]);
					

					//SI SE CUMPLE ESTA CONDICIÓN QUIERE DECIR QUE NO HA RECIBIDO TODOS LOS MENSAJES CON LOS DIVISORES DE ESE PROCESO
					while(procesos[status.MPI_SOURCE-1].sumaParcialTotal>procesos[status.MPI_SOURCE-1].sumaDivisores)
					{
						MPI_Recv(&acum,1,MPI_LONG,status.MPI_SOURCE,divisorEncontrado, MPI_COMM_WORLD,&status2);
						procesos[status.MPI_SOURCE-1].sumaDivisores+=acum;
						procesos[status.MPI_SOURCE-1].contNumDivisores++;
						acumSumaTotal+=acum;
					}
				}

				if(status.MPI_TAG==envioSumaTotal)
				{
					flagSumTotalUltProc=1;
					sumTotalUltProc=acum;
				}
		}

		tFin=MPI_Wtime();
		//AHORA RECIBIREMOS EL MENSAJE DEL PROCESO ÚLTIMO QUE NOS ENVÍA LA SUMA TOTAL CALCULADA POR EL EN BASE A LAS SUMAS PARCIALES QUE LE HAN IDO LLEGANDO TAMBIÉN DEL RESTO DE PROCESOS
		//SOLO SI NO LO HEMOS RECIBIDO ANTES
		if(!flagSumTotalUltProc)
			MPI_Recv(&sumTotalUltProc,1,MPI_LONG,(nprc-1),envioSumaTotal, MPI_COMM_WORLD,&status2);



		if(sumTotalUltProc==acumSumaTotal)
		{
	     	printf("\nSUMA TOTAL OK\nCALCULADA --> %ld\nRECIBIDA  --> %ld\n\n",acumSumaTotal,sumTotalUltProc);
			printf("Proceso\t| Nº Divisores | Suma\t| Tpo calculo\n");
			printf("--------------------------------------------------------------------\n");

			for(int i=0; i<nprc-1; i++)
			{
	      		printf(" %d\t|\t%d\t|%ld |\t%f|\n" ,i+1,procesos[i].contNumDivisores,procesos[i].sumaParcialTotal,procesos[i].tiempoEjecucion);
	      		contTotalDivisores+=procesos[i].contNumDivisores;
	      	}
	     	printf("--------------------------------------------------------------------\n");
	     	printf("TOTAL  |\t%d\t| %ld |\n\n\n",contTotalDivisores,acumSumaTotal);

	     	
	     	if(acumSumaTotal==valor && valor!=0 && valor!=1 && valor>0)
	     	{
	     		printf("EL NUMERO %ld ES UN NUMERO PERFECTO\n",valor);
	     		
	     	}

	     	else if(acumSumaTotal<valor  && valor>0)
	     	{
	     		printf("EL NUMERO %ld NO ES PERFECTO, ES DEFECTIVO\n",valor);
     		}
	   
	     	else if(acumSumaTotal>valor  && valor>0)
	     	{
	     		printf("EL NUMERO %ld NO ES PERFECTO, ES EXCESIVO\n",valor);
	   		}

	   		else
	   		{
	   			printf("ERROR: EL NÚMERO %ld NO SE CORRESPONDE CON NINGUNA DE LAS OPCIONES\n",valor);
	   		}

	   		printf("Numero de procesos: %d\n",nprc);
	     	printf("Numero de procesos buscadores: %d\n" ,nprc-1);
	     	printf("Tiempo total de procesamiento: %f\n\n",(float)(tFin-tInitTotal)); 
     	}

     	else
     	{
     		printf("ERROR: Los sumatorios del proceso 0 (%ld) y el ultimo proceso (%ld) no son iguales\n",acumSumaTotal,sumTotalUltProc);
     	}
	}

MPI_Finalize();
}