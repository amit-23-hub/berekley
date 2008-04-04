/**CFile****************************************************************

  FileName    [ifLib.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    [LUT library.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: ifLib.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/

#include "if.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline char * If_UtilStrsav( char *s ) {  return !s ? s : strcpy(ALLOC(char, strlen(s)+1), s);  }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reads the description of LUTs from the LUT library file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Lib_t * If_LutLibRead( char * FileName )
{
    char pBuffer[1000], * pToken;
    If_Lib_t * p;
    FILE * pFile;
    int i, k;

    pFile = fopen( FileName, "r" );
    if ( pFile == NULL )
    {
        printf( "Cannot open LUT library file \"%s\".\n", FileName );
        return NULL;
    }

    p = ALLOC( If_Lib_t, 1 );
    memset( p, 0, sizeof(If_Lib_t) );
    p->pName = If_UtilStrsav( FileName );

    i = 1;
    while ( fgets( pBuffer, 1000, pFile ) != NULL )
    {
        pToken = strtok( pBuffer, " \t\n" );
        if ( pToken == NULL )
            continue;
        if ( pToken[0] == '#' )
            continue;
        if ( i != atoi(pToken) )
        {
            printf( "Error in the LUT library file \"%s\".\n", FileName );
            free( p );
            return NULL;
        }

        // read area
        pToken = strtok( NULL, " \t\n" );
        p->pLutAreas[i] = (float)atof(pToken);

        // read delays
        k = 0;
        while ( pToken = strtok( NULL, " \t\n" ) )
            p->pLutDelays[i][k++] = (float)atof(pToken);

        // check for out-of-bound
        if ( k > i )
        {
            printf( "LUT %d has too many pins (%d). Max allowed is %d.\n", i, k, i );
            return NULL;
        }

        // check if var delays are specified
        if ( k > 1 )
            p->fVarPinDelays = 1;

        if ( i == IF_MAX_LUTSIZE )
        {
            printf( "Skipping LUTs of size more than %d.\n", i );
            return NULL;
        }
        i++;
    }
    p->LutMax = i-1;

    // check the library
    if ( p->fVarPinDelays )
    {
        for ( i = 1; i <= p->LutMax; i++ )
            for ( k = 0; k < i; k++ )
            {
                if ( p->pLutDelays[i][k] <= 0.0 )
                    printf( "Warning: Pin %d of LUT %d has delay %f. Pin delays should be non-negative numbers. Technology mapping may not work correctly.\n", 
                        k, i, p->pLutDelays[i][k] );
                if ( k && p->pLutDelays[i][k-1] > p->pLutDelays[i][k] )
                    printf( "Warning: Pin %d of LUT %d has delay %f. Pin %d of LUT %d has delay %f. Pin delays should be in non-decreasing order. Technology mapping may not work correctly.\n", 
                        k-1, i, p->pLutDelays[i][k-1], 
                        k, i, p->pLutDelays[i][k] );
            }
    }
    else
    {
        for ( i = 1; i <= p->LutMax; i++ )
        {
            if ( p->pLutDelays[i][0] <= 0.0 )
                printf( "Warning: LUT %d has delay %f. Pin delays should be non-negative numbers. Technology mapping may not work correctly.\n", 
                    k, i, p->pLutDelays[i][0] );
        }
    }

    return p;
}

/**Function*************************************************************

  Synopsis    [Duplicates the LUT library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Lib_t * If_LutLibDup( If_Lib_t * p )
{
    If_Lib_t * pNew;
    pNew = ALLOC( If_Lib_t, 1 );
    *pNew = *p;
    pNew->pName = If_UtilStrsav( pNew->pName );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Frees the LUT library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_LutLibFree( If_Lib_t * pLutLib )
{
    if ( pLutLib == NULL )
        return;
    FREE( pLutLib->pName );
    FREE( pLutLib );
}


/**Function*************************************************************

  Synopsis    [Prints the LUT library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_LutLibPrint( If_Lib_t * pLutLib )
{
    int i, k;
    printf( "# The area/delay of k-variable LUTs:\n" );
    printf( "# k    area     delay\n" );
    if ( pLutLib->fVarPinDelays )
    {
        for ( i = 1; i <= pLutLib->LutMax; i++ )
        {
            printf( "%d   %7.2f  ", i, pLutLib->pLutAreas[i] );
            for ( k = 0; k < i; k++ )
                printf( " %7.2f", pLutLib->pLutDelays[i][k] );
            printf( "\n" );
        }
    }
    else
        for ( i = 1; i <= pLutLib->LutMax; i++ )
            printf( "%d   %7.2f   %7.2f\n", i, pLutLib->pLutAreas[i], pLutLib->pLutDelays[i][0] );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the delays are discrete.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_LutLibDelaysAreDiscrete( If_Lib_t * pLutLib )
{
    float Delay;
    int i;
    for ( i = 1; i <= pLutLib->LutMax; i++ )
    {
        Delay = pLutLib->pLutDelays[i][0];
        if ( ((float)((int)Delay)) != Delay )
            return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Sets simple LUT library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Lib_t * If_SetSimpleLutLib( int nLutSize )
{
    If_Lib_t s_LutLib10= { "lutlib",10, 0, {0,1,1,1,1,1,1,1,1,1,1}, {{0},{1},{1},{1},{1},{1},{1},{1},{1},{1},{1}} };
    If_Lib_t s_LutLib9 = { "lutlib", 9, 0, {0,1,1,1,1,1,1,1,1,1}, {{0},{1},{1},{1},{1},{1},{1},{1},{1},{1}} };
    If_Lib_t s_LutLib8 = { "lutlib", 8, 0, {0,1,1,1,1,1,1,1,1}, {{0},{1},{1},{1},{1},{1},{1},{1},{1}} };
    If_Lib_t s_LutLib7 = { "lutlib", 7, 0, {0,1,1,1,1,1,1,1}, {{0},{1},{1},{1},{1},{1},{1},{1}} };
    If_Lib_t s_LutLib6 = { "lutlib", 6, 0, {0,1,1,1,1,1,1}, {{0},{1},{1},{1},{1},{1},{1}} };
    If_Lib_t s_LutLib5 = { "lutlib", 5, 0, {0,1,1,1,1,1}, {{0},{1},{1},{1},{1},{1}} };
    If_Lib_t s_LutLib4 = { "lutlib", 4, 0, {0,1,1,1,1}, {{0},{1},{1},{1},{1}} };
    If_Lib_t s_LutLib3 = { "lutlib", 3, 0, {0,1,1,1}, {{0},{1},{1},{1}} };
    If_Lib_t * pLutLib;
    assert( nLutSize >= 3 && nLutSize <= 10 );
    switch ( nLutSize )
    {
        case 3:  pLutLib = &s_LutLib3; break;
        case 4:  pLutLib = &s_LutLib4; break;
        case 5:  pLutLib = &s_LutLib5; break;
        case 6:  pLutLib = &s_LutLib6; break;
        case 7:  pLutLib = &s_LutLib7; break;
        case 8:  pLutLib = &s_LutLib8; break;
        case 9:  pLutLib = &s_LutLib9; break;
        case 10: pLutLib = &s_LutLib10; break;
        default: pLutLib = NULL; break;
    }
    if ( pLutLib == NULL )
        return NULL;
    return If_LutLibDup(pLutLib);
}

/**Function*************************************************************

  Synopsis    [Gets the delay of the fastest pin.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float If_LutLibFastestPinDelay( If_Lib_t * p )
{
    return !p? 1.0 : p->pLutDelays[p->LutMax][0];
}

/**Function*************************************************************

  Synopsis    [Gets the delay of the slowest pin.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float If_LutLibSlowestPinDelay( If_Lib_t * p )
{
    return !p? 1.0 : (p->fVarPinDelays? p->pLutDelays[p->LutMax][p->LutMax-1]: p->pLutDelays[p->LutMax][0]);
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


