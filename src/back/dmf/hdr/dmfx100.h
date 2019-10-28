/*
** Copyright (c) 2011, 2011 Actian Corporation
**
**
*/

/**
** Name: DMFX100.H - DMF x100 definitions.
**
** Description:
**      This file contains the structures and constants used by dmf x100
**	support routines.
**
** History:
**      23-jun-2011 (stial01)
**          Created.
*/

/* Function prototye definitions */
FUNC_EXTERN DB_STATUS x100_terminate(DMF_JSX	*jsx, DMP_DCB     *dcb);
FUNC_EXTERN DB_STATUS x100_stall(DMF_JSX	*jsx, DMP_DCB     *dcb);
FUNC_EXTERN DB_STATUS x100_unstall(DMF_JSX	*jsx, DMP_DCB     *dcb);
