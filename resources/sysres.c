/*
 * System resources loading
 *
 * Copyright 1995 Alexandre Julliard
 */

#include "windows.h"
#include "global.h"
#include "options.h"
#include "resource.h"

#include "sysres_En.h"
#include "sysres_Es.h"
#include "sysres_De.h"
#include "sysres_No.h"
#include "sysres_Fr.h"
#include "sysres_Fi.h"
#include "sysres_Da.h"
#include "sysres_Cz.h"
#include "sysres_Eo.h"
#include "sysres_It.h"
#include "sysres_Ko.h"
#include "sysres_Hu.h"
#include "sysres_Pl.h"
#include "sysres_Po.h"

static const struct resource * const * SYSRES_Resources[] =
{
    sysres_En_Table,  /* LANG_En */
    sysres_Es_Table,  /* LANG_Es */
    sysres_De_Table,  /* LANG_De */
    sysres_No_Table,  /* LANG_No */
    sysres_Fr_Table,  /* LANG_Fr */
    sysres_Fi_Table,  /* LANG_Fi */
    sysres_Da_Table,  /* LANG_Da */
    sysres_Cz_Table,  /* LANG_Cz */
    sysres_Eo_Table,  /* LANG_Eo */
    sysres_It_Table,  /* LANG_It */
    sysres_Ko_Table,  /* LANG_Ko */
    sysres_Hu_Table,  /* LANG_Hu */
    sysres_Pl_Table,  /* LANG_Pl */
    sysres_Po_Table   /* LANG_Po */
};


/***********************************************************************
 *           SYSRES_GetResourcePtr
 *
 * Return a pointer to a system resource.
 */
LPCVOID SYSRES_GetResPtr( SYSTEM_RESOURCE id )
{
    return SYSRES_Resources[Options.language][id]->bytes;
}


/***********************************************************************
 *           SYSRES_LoadResource
 *
 * Create a global memory block for a system resource.
 */
HGLOBAL16 SYSRES_LoadResource( SYSTEM_RESOURCE id )
{
    const struct resource *resPtr;

    resPtr = SYSRES_Resources[Options.language][id];
    return GLOBAL_CreateBlock( GMEM_FIXED, resPtr->bytes, resPtr->size,
			       GetCurrentPDB(), FALSE, FALSE, TRUE, NULL );
}


/***********************************************************************
 *           SYSRES_FreeResource
 *
 * Free a global memory block for a system resource.
 */
void SYSRES_FreeResource( HGLOBAL16 handle )
{
    GLOBAL_FreeBlock( handle );
}
