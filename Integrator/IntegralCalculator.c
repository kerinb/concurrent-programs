#include <pthread.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <math.h>

typedef struct Integral // iformation requried to integrate
{
	unsigned int threadNumber; // number of threads used
	double	LowerLimit; // lower limit consider on x-axis
	double	UpperLimit; // upper limit consider on x-axos
	unsigned int NumberOfSlices; // number of divisions used for each thread
	double Result; // integral
} Integral;

#define NUM_THREADS 4
#define BILLION  1E9;

// function containing the function to be intefrated 
double func_(double x) 
{ 
        return 4/(1+pow(x,2));
}

void *calculate_delta_area(void *args) 
{
	Integral *integral = (Integral*)args;	
	double delta = (integral->UpperLimit-integral->LowerLimit)/integral->NumberOfSlices; //calculate the delta_x value
	double Result = 0;
	double x_pos=integral->LowerLimit; // start at the lower limit and work way up to upper limit
		
	for (unsigned int i=0;i<integral->NumberOfSlices;i++) 
	{
		Result += delta*(func_(x_pos)+func_(x_pos+delta))/2; // calculate the area between two delta_x points
		x_pos+=delta;// move onto the next position
	}
	
	integral->Result=Result;
	pthread_exit(NULL); 
}

int main (int argc, const char *argv[]) 
{
        struct timespec requestStart, requestEnd;

	const int totalNumberOfSlices=10000000; // number of division per thread
	const double LowerLimit = 0; // lower limit on x-axis
	const double UpperLimit = 1; // upper limit on x-axis
	double finalResult = 0;
	
	Integral Integral_Thread[NUM_THREADS]; // array of type Integral( data required to integrate a function)
	pthread_t threads[NUM_THREADS]; //array of threads
	
	int rc,t; 
        clock_gettime(CLOCK_REALTIME, &requestStart);
	for (t=0;t<NUM_THREADS;t++) // for each thread 
	{ 
		Integral_Thread[t].threadNumber=t; // set the thread ID 
		Integral_Thread[t].LowerLimit = LowerLimit+((UpperLimit-LowerLimit)/NUM_THREADS)*t; //set the lower limit of the current thread
		Integral_Thread[t].UpperLimit = Integral_Thread[t].LowerLimit+((UpperLimit-LowerLimit)/NUM_THREADS); //set the upper limit of the current thread
		Integral_Thread[t].NumberOfSlices = totalNumberOfSlices/NUM_THREADS; // set the number of threads required per division
		rc = pthread_create(&threads[t],NULL, calculate_delta_area,(void *)&Integral_Thread[t]); // 
		
		if (rc) 
		{ 
			printf("ERROR return code from pthread_create(): %d\n",rc); 
			return(1); 
		} 
	} 
	// wait for threads to exit 
	for(t=0;t<NUM_THREADS;t++)  
		pthread_join( threads[t], NULL); 
	
	for(t=0;t<NUM_THREADS;t++) 
		finalResult+=Integral_Thread[t].Result;
	
	if (finalResult < 0)
	        finalResult = finalResult*-1;
        clock_gettime(CLOCK_REALTIME, &requestEnd);
	
	printf("Running on %d thread(s), result is %lf.\n",NUM_THREADS,finalResult);
	
	// Calculate time it took
        double accum = (requestEnd.tv_sec-requestStart.tv_sec)+(requestEnd.tv_nsec-requestStart.tv_nsec)/BILLION;
        printf( "%lf ns\n", accum );
		
	return(0); 
}
