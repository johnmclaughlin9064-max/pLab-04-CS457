/*-------------------------------------------------------------------------------

FILE:   dispatcher.c

Written By: John McLaughlin and Kyle Mirra
     1- Dr. Mohamed Aboutabl
Submitted on: 10/31/2026
-------------------------------------------------------------------------------*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "wrappers.h"

#define   READ_END	0
#define   WRITE_END	1
#define   STDIN  0
#define   STDOUT 1
//--------------------------------------------------------------------------
int main( int argc , char *argv[] )
{
    pid_t  amalPID , basimPID, kdcPID; 
    int    AmalToBasim[2] , BasimToAmal[2] , AmalToKDC[2], KDCToAmal[2];  // Amal to Basim control and data pipes
    char   arg1[20] , arg2[20] , arg3[20], arg4[20];
    
    Pipe( AmalToBasim) ;  // create pipe for Amal-to-Basim control
    Pipe( BasimToAmal) ;  // create pipe for Amal-to-Basim data
    Pipe( AmalToKDC);
    Pipe(KDCToAmal);

    printf("\tDispatcher started and created these pipes\n") ;
    printf("\tAmal-to-KDC protocol pipe: read=%d  write=%d\n", AmalToKDC[ READ_END ] , AmalToKDC[ WRITE_END ] ) ;
    printf("\tKDC-to-Amal protocol pipe: read=%d  write=%d\n", KDCToAmal[ READ_END ] , KDCToAmal[ WRITE_END ] ) ;
    printf("\tAmal-to-Basim protocol pipe: read=%d  write=%d\n", AmalToBasim[ READ_END ] , AmalToBasim[ WRITE_END ] ) ;
    printf("\tBasim-to-Amal prtocol pipe: read=%d  write=%d\n", BasimToAmal[ READ_END ] , BasimToAmal[ WRITE_END ] ) ;


    // Create both child processes:
    amalPID = Fork() ;
    if ( amalPID == 0 )
    {    
        // This is the Amal process.
        // Amal will not use these ends of the pipes, decrement their 'Ref Count'
        close( AmalToBasim[ READ_END ] ) ;
        close( BasimToAmal[ WRITE_END ] ) ;
        close(AmalToKDC[READ_END]);
        close(KDCToAmal[WRITE_END]);
        
        // Prepare the file descriptors as args to Amal
        snprintf( arg1 , 20 , "%d" , KDCToAmal[ READ_END ] ) ;
        snprintf( arg2 , 20 , "%d" , AmalToKDC[ WRITE_END ] ) ;
        snprintf( arg1 , 20 , "%d" , BasimToAmal[ READ_END ] ) ;
        snprintf( arg2 , 20 , "%d" , AmalToBasim[ WRITE_END ] ) ;
        
        // Now, Start Amal
        char * cmnd = "./amal/amal" ;
        execlp( cmnd , "Amal" , arg1 , arg2 , arg3, arg4, NULL );

        // the above execlp() only returns if an error occurs
        perror("ERROR starting Amal" );
        exit(-1) ;      
    } 
    
    // This is still the Dispatcher process 
    // Create Basim
    basimPID = Fork() ;
    if ( basimPID == 0 )
    {  
        // This is the Basim process
        // Basim will not use these ends of the pipes, decrement their 'count'
        close( AmalToBasim[ WRITE_END ] ) ;
        close( BasimToAmal[ READ_END  ] ) ;
        close( AmalToKDC  [ READ_END  ] ) ;
        close( AmalToKDC  [ WRITE_END ] ) ;
        close( KDCToAmal  [ READ_END  ] ) ;
        close( KDCToAmal  [ WRITE_END ] ) ;
            
        // Prepare the file descriptors as args to Basim
        snprintf( arg1 , 20 , "%d" , AmalToBasim[ READ_END ] ) ;
        snprintf( arg2 , 20 , "%d" , BasimToAmal[ WRITE_END ] ) ;

        char * cmnd = "./basim/basim" ;
        execlp( cmnd , "Basim" , arg1 , arg2 , NULL );

        // the above execlp() only returns if an error occurs
        perror("ERROR starting Basim" ) ;
        exit(-1) ;
    }

    // This is still the parent Dispatcher  process
    // Create the KDC
    kdcPID = Fork();
    if (kdcPID == 0)
    {
        // This is the KDC process
        // Closed the unused ends
        close( AmalToKDC  [ WRITE_END ] ) ;
        close( KDCToAmal  [ READ_END  ] ) ;
        close( AmalToBasim[ READ_END  ] ) ;
        close( AmalToBasim[ WRITE_END ] ) ;
        close( BasimToAmal[ READ_END  ] ) ;
        close( BasimToAmal[ WRITE_END ] ) ;
 
        snprintf( arg1 , 20 , "%d" , AmalToKDC[ READ_END  ] ) ;     
        snprintf( arg2 , 20 , "%d" , KDCToAmal[ WRITE_END ] ) ;     

        char * cmnd = "./kdc/kdc" ;
        execlp( cmnd , "KDC" , arg1 , arg2 , NULL ) ;
 
        // the above execlp() only returns if an error occurs
        perror("ERROR starting KDC" ) ;
        exit(-1) ;
    }


    // This is still the dispatcher
    // Close all ends of all pipes
    close( AmalToBasim[ READ_END  ] ) ;
    close( AmalToBasim[ WRITE_END ] ) ;
    close( BasimToAmal[ READ_END  ] ) ;
    close( BasimToAmal[ WRITE_END ] ) ;
    close( AmalToKDC  [ READ_END  ] ) ;
    close( AmalToKDC  [ WRITE_END ] ) ;
    close( KDCToAmal  [ READ_END  ] ) ;
    close( KDCToAmal  [ WRITE_END ] ) ;  

    int  exitStatus ;

    printf("\n\tDispatcher is now waiting for Amal to terminate\n") ;
    waitpid( amalPID , &exitStatus , 0 ) ;
    printf("\n\tAmal terminated ... "  ) ;
	if (  WIFEXITED( exitStatus ) )
            printf(" with status =%d\n" , WEXITSTATUS(exitStatus ) ) ;

    printf("\n\tDispatcher is now waiting for Basim to terminate\n") ;
    waitpid( basimPID , &exitStatus , 0 ) ;
    printf("\n\tBasim terminated ... " ) ;
	if (  WIFEXITED( exitStatus ) )
        printf(" with status =%d\n" , WEXITSTATUS(exitStatus ) ) ;

    printf("\n\tDispatcher is now waiting for the KDC to terminate\n") ;
    waitpid( kdcPID , &exitStatus , 0 ) ;
    printf("\n\tKDC terminated ... " ) ;
    if ( WIFEXITED( exitStatus ) )
        printf(" with status =%d\n" , WEXITSTATUS( exitStatus ) ) ;

    return 0; 
}

