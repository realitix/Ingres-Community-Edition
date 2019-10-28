/*
** Copyright (c) 2004 Actian Corporation
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iiapi.h>
#include <sapiw.h>


/*
** Name:	testsapi.c - test Synchronous OpenAPI wrapper functions
**
** Description:
**	This exercises several of the Sync API functions. It also serves as
**	a sample program to show how to use the macros and call the functions.
*/
int
main(int argc, char *argv[])
{
	II_PTR connH, tranH, stmtH;
	IIAPI_GETEINFOPARM errp;
	IIAPI_STATUS status;
	IIAPI_DESCRIPTOR adesc[3];
	IIAPI_DATAVALUE adata[3];
	int col1 = 123;
	char col2[13];
	double col3 = 987.65;

	if (argc != 2)
	{
		fprintf(stderr, "usage: testsapi dbname\n");
		exit(EXIT_FAILURE);
	}

	status = IIsw_initialize(NULL);
	if (status != IIAPI_ST_SUCCESS)
	{
		fprintf(stderr, "Error initializing. Status = %d\n", status);
		exit(EXIT_FAILURE);
	}

	connH = NULL;
	status = IIsw_dbConnect(argv[1], NULL, NULL, &connH, NULL, &errp);
	if (status != IIAPI_ST_SUCCESS)
	{
		fprintf(stderr, "Error connecting. Status = %d\n", status);
		exit(EXIT_FAILURE);
	}

	tranH = NULL;
	status = IIsw_query(connH, &tranH,
		"CREATE TABLE testsapi (col1 INTEGER NOT NULL, col2 CHAR(12) \
		NOT NULL NOT DEFAULT, col3 FLOAT NOT NULL DEFAULT 0)", 0,
		NULL, NULL, NULL, NULL, &stmtH, &errp);
	IIsw_close(stmtH, &errp);
	if (status != IIAPI_ST_SUCCESS)
	{
		fprintf(stderr, "Error creating table. Status = %d\n", status);
		exit(EXIT_FAILURE);
	}
	status = IIsw_commit(&tranH, &errp);
	if (status != IIAPI_ST_SUCCESS)
	{
		fprintf(stderr, "Error committing. Status = %d\n", status);
		exit(EXIT_FAILURE);
	}

	strcpy(col2, "a test      ");
	SW_PARM_DESC_DATA_INIT(adesc[0], adata[0], IIAPI_INT_TYPE, col1);
	SW_PARM_DESC_DATA_INIT(adesc[1], adata[1], IIAPI_CHA_TYPE, col2);
	SW_PARM_DESC_DATA_INIT(adesc[2], adata[2], IIAPI_FLT_TYPE, col3);
	status = IIsw_query(connH, &tranH,
		"INSERT INTO testsapi VALUES ( ~V , ~V , ~V )", 3, adesc,
		adata, NULL, NULL, &stmtH, &errp);
	IIsw_close(stmtH, &errp);
	if (status != IIAPI_ST_SUCCESS)
	{
		fprintf(stderr, "Error inserting. Status = %d\n", status);
		exit(EXIT_FAILURE);
	}

	status = IIsw_query(connH, &tranH,
		"UPDATE testsapi SET col1 = col1 + 198, col3 = col3 + 246.91",
		0, NULL, NULL, NULL, NULL, &stmtH, &errp);
	IIsw_close(stmtH, &errp);
	if (status != IIAPI_ST_SUCCESS)
	{
		fprintf(stderr, "Error updating. Status = %d\n", status);
		exit(EXIT_FAILURE);
	}

	col1 = 0;
	*col2 = '\0';
	col3 = 0.0;
	SW_COLDATA_INIT(adata[0], col1);
	SW_COLDATA_INIT(adata[1], col2);
	SW_COLDATA_INIT(adata[2], col3);
	stmtH = NULL;
	while (1)
	{
		status = IIsw_selectLoop(connH, &tranH,
			"SELECT * FROM testsapi", 0, NULL, NULL, 3, adata,
			&stmtH, &errp);
		if (status == IIAPI_ST_NO_DATA || status != IIAPI_ST_SUCCESS)
			break;
		SW_CHA_TERM(col2, adata[1]);
		printf("col1 = %7d | col2 = %s | col3 = %f\n", col1, col2,
			col3);
	}
	if (status != IIAPI_ST_NO_DATA && status != IIAPI_ST_SUCCESS)
	{
		fprintf(stderr, "Error selecting. Status = %d\n", status);
		exit(EXIT_FAILURE);
	}
	IIsw_close(stmtH, &errp);
	status = IIsw_rollback(&tranH, &errp);

	status = IIsw_query(connH, &tranH, "DROP TABLE testsapi", 0,
		NULL, NULL, NULL, NULL, &stmtH, &errp);
	IIsw_close(stmtH, &errp);
	if (status != IIAPI_ST_SUCCESS)
	{
		fprintf(stderr, "Error dropping table. Status = %d\n", status);
		exit(EXIT_FAILURE);
	}
	status = IIsw_commit(&tranH, &errp);

	status = IIsw_disconnect(&connH, &errp);
	if (status != IIAPI_ST_SUCCESS)
	{
		fprintf(stderr, "Error disconnecting. Status = %d\n", status);
		exit(EXIT_FAILURE);
	}
	status = IIsw_terminate(NULL);
	if (status != IIAPI_ST_SUCCESS)
	{
		fprintf(stderr, "Error terminating. Status = %d\n", status);
		exit(EXIT_FAILURE);
	}

	printf("Successful completion\n");
	return (0);
}
