/*
 ***************************************************************************
 * MediaTek Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 1997-2012, MediaTek, Inc.
 *
 * All rights reserved. MediaTek source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of MediaTek. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of MediaTek Technology, Inc. is obtained.
 ***************************************************************************

*/

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/
#define PATH_OF_SKU_TABLE_IN   "/txpwr/sku_tables/"
#define PATH_OF_SKU_TABEL_OUT  "/include/txpwr/"

#define INPUT_FILE_PATH_SIZE        512
#define OUTPUT_FILE_PATH_SIZE       512
#define DEFAULT_FILE_PATH_SIZE      512
#define VARIABLE_NAME_SIZE           64

#define MAX_SKUTABLE_NUM             20

/*******************************************************************************
 *    TYPES
 ******************************************************************************/
typedef enum _POWER_LIMIT_FILE_TYPE {
	INPUT_FILE = 0,
	OUTPUT_FILE,
	DEFAULT_FILE,
	FILE_TYPE_NUM
} POWER_LIMIT_FILE_TYPE, *P_POWER_LIMIT_FILE_TYPE;

typedef enum _POWER_LIMIT_TABLE {
	POWER_LIMIT_TABLE_TYPE_SKU = 0,
	POWER_LIMIT_TABLE_TYPE_BACKOFF,
	POWER_LIMIT_TABLE_TYPE_NUM
} POWER_LIMIT_TABLE, *P_POWER_LIMIT_TABLE;

typedef enum _FILE_TRANSFORM_STATE {
	FILE_TRANSFORM_PARSING_STATE = 0,
	FILE_TRANSFORM_CHECK_STATE,
	FILE_TRANSFORM_STATE_NUM
} FILE_TRANSFORM_STATE, *P_FILE_TRANSFORM_STATE;

/*******************************************************************************
 *    PUBLIC DATA
 ******************************************************************************/
char **_PwrLimitType;

/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/
int DataToHeaderTrans(char *infname, char *outfname, char *varname, char *deffname, const char *mode)
{
	int ret = 0;
	FILE *infile, *outfile;
	char c;
	int fgDefTable = 0;
	int u1State = 0;

	/* Open input file */
	infile = fopen(infname, "r");

	/* Check open file status for input file */
	if (infile == (FILE *) NULL) {
		printf("Can't read file %s\n", infname);
		printf("System would automatically apply default table !!\n");

		/* Flag for use Default SKU table */
		fgDefTable = 1;

		/* Open default input file */
		infile = fopen(deffname, "r");

		/* Check open file status for default file */
		if (infile == (FILE *) NULL) {
			printf("Can't read default file %s\n", deffname);
			return -1;
		}
	}

	/* Open output file */
	outfile = fopen(outfname, mode);

	/* Check open file status for output file */
	if (outfile == (FILE *) NULL) {
		printf("Can't open write file %s\n", outfname);

		/* Close input file */
		fclose(infile);
		return -1;
	}

	/* contents in header file */
	fprintf(outfile, "char %s[] = \"", varname);

	while (1) {
		char cTempStringBuffer, cTempStringBuffer2;
		char cString[1];

		/* read one character from input file */
		c = getc(infile);

		/* end-of-file check */
		if (feof(infile))
			break;

		/* Finite State Machine for string parsing */
		switch (u1State) {
		case FILE_TRANSFORM_PARSING_STATE:
			if (c == ',') {
				/* state transition to check state (comma symbol is needed to be removed in header file) */
				u1State = FILE_TRANSFORM_CHECK_STATE;
			} else {
				if (c == '\n') {
					/* new line symbol handle */
					fputs("\t\"\n\"", outfile);
				} else {
					sprintf(cString, "%c", c);
					fputs(cString, outfile);
				}
			}
			break;

		case FILE_TRANSFORM_CHECK_STATE:
			if (c != ',') {

				/* state transition to parsing state */
				u1State = FILE_TRANSFORM_PARSING_STATE;

				/* backup character to temp buffer variable (prevent buffer used in "getc" to be re-assigned for function "sprintf") */
				cTempStringBuffer2 = c;

				if (c == '\n') {
					fputs("\t\"\n\"", outfile);
				} else {
					fputs(" ", outfile);
					sprintf(cString, "%c", cTempStringBuffer2);
					fputs(cString, outfile);
				}
			}
			break;

		default:
			break;
		}
	}

	/* end-of-file symbol processing */
	fputs("\"", outfile);
	fputs(";", outfile);
	fputs("\n", outfile);

	/* end with new line for different variables */
	fputs("\n", outfile);

	/* close input file */
	fclose(infile);

	/* close output file */
	fclose(outfile);
}

int PowerLimitTypeDefine(void)
{
	char **PowerLimitType;
	char *Sku, *Backoff;
	char SkuString[4] = "Sku";
	char BackoffString[8] = "Backoff";

	/* allocate memory for pointer array for Power Limit Type string */
	PowerLimitType = (char **)malloc(sizeof(char *)*POWER_LIMIT_TABLE_TYPE_NUM);

	/* allocate memory for Sku string */
	Sku = (char *)malloc(sizeof(char)*(strlen(SkuString) + 1));
	strcpy(Sku, SkuString);
	*PowerLimitType = Sku;

	/* allocate memory for Backoff string */
	Backoff = (char *)malloc(sizeof(char)*(strlen(BackoffString) + 1));
	strcpy(Backoff, BackoffString);
	*(PowerLimitType + 1) = Backoff;

	/* Link pointer array to global pointer for further reference */
	_PwrLimitType = PowerLimitType;
}

void PowerLimitTypeFree(void)
{
	char **PowerLimitType;
	char *Sku, *Backoff;
	char SkuString[4] = "Sku";
	char BackoffString[8] = "Backoff";

	/* get pointer array from global pointer */
	PowerLimitType = _PwrLimitType;

	/* free memory for Sku String */
	free(*PowerLimitType);

	/* free memory for Backoff String */
	free(*(PowerLimitType + 1));

	/* free memory for pointer array for Power Limit Type string */
	free(PowerLimitType);
}

char *PowerLimitTypeString(int u4Type)
{
	/* get pointer array from global pointer */
	char **PowerLimitType;
	PowerLimitType = _PwrLimitType;

	return *(PowerLimitType + u4Type);
}

void FilePathArrangeProc(char *pcFilePath, int u4FilePathSize, char *EnvironDirectoryPath, char *DirectoryPath, int u4Type, int u4PwrLimitType, int u4PwrLimitTblIdx)
{
	char cTempStringBuffer[20];

	/* configure file address and file name */
	memset(pcFilePath, 0, u4FilePathSize);
	strcat(pcFilePath, EnvironDirectoryPath);
	strcat(pcFilePath, DirectoryPath);

	switch (u4Type) {
	case INPUT_FILE:
		strcat(pcFilePath, PowerLimitTypeString(u4PwrLimitType));
		strcat(pcFilePath, "_");
		sprintf(cTempStringBuffer, "%02d", u4PwrLimitTblIdx);
		strcat(pcFilePath, cTempStringBuffer);
		strcat(pcFilePath, ".dat");
		printf("Input: [%d] %s\n", u4PwrLimitTblIdx, pcFilePath);
		break;

	case OUTPUT_FILE:
		strcat(pcFilePath, "PowerLimit_mt7915.h");
		printf("Output: %s\n", pcFilePath);
		break;

	case DEFAULT_FILE:
		strcat(pcFilePath, PowerLimitTypeString(u4PwrLimitType));
		strcat(pcFilePath, "_default.dat");
		printf("Def Input: [%d] %s\n", u4PwrLimitTblIdx, pcFilePath);
		break;

	default:
		break;
	}
}

void VariableNameArrangeProc(char *pcVariableName, int u4VariableNameSize, int u4PwrLimitType, int u4PwrLimitTblIdx)
{
	char cTempStringBuffer[20];

	memset(pcVariableName, 0, u4VariableNameSize);
	strcat(pcVariableName, PowerLimitTypeString(u4PwrLimitType));
	strcat(pcVariableName, "_");
	sprintf(cTempStringBuffer, "%02d", u4PwrLimitTblIdx);
	strcat(pcVariableName, cTempStringBuffer);
}

int main(int argc, char *argv[])
{
	char infname[INPUT_FILE_PATH_SIZE];
	char outfname[OUTPUT_FILE_PATH_SIZE];
	char deffname[DEFAULT_FILE_PATH_SIZE];
	char varname[VARIABLE_NAME_SIZE];
	char *rt28xxdir;
	int  u4PwrLimitTblIdx = 0, u4PwrLimitType = 0;
	int  fgStart;

	/* Power Limit Type String Define */
	PowerLimitTypeDefine();

	/* get directory path from environemnt variable */
	rt28xxdir = (char *)getenv("RT28xx_DIR");

	/* configure output file address and file name */
	FilePathArrangeProc(outfname, OUTPUT_FILE_PATH_SIZE, rt28xxdir, PATH_OF_SKU_TABEL_OUT, OUTPUT_FILE, u4PwrLimitType, u4PwrLimitTblIdx);

	/* Trasform SKU table data file to header file */
	for (u4PwrLimitType = 0, fgStart = 1; u4PwrLimitType < POWER_LIMIT_TABLE_TYPE_NUM; u4PwrLimitType++) {
		for (u4PwrLimitTblIdx = 1; u4PwrLimitTblIdx <= MAX_SKUTABLE_NUM; u4PwrLimitTblIdx++) {
			/* configure input file address and file name */
			FilePathArrangeProc(infname, INPUT_FILE_PATH_SIZE, rt28xxdir, PATH_OF_SKU_TABLE_IN, INPUT_FILE, u4PwrLimitType, u4PwrLimitTblIdx);

			/* configure default input file address and file name */
			FilePathArrangeProc(deffname, DEFAULT_FILE_PATH_SIZE, rt28xxdir, PATH_OF_SKU_TABLE_IN, DEFAULT_FILE, u4PwrLimitType, u4PwrLimitTblIdx);

			/* Configure variable name for SKU contents in header file */
			VariableNameArrangeProc(varname, VARIABLE_NAME_SIZE, u4PwrLimitType, u4PwrLimitTblIdx);

			/* Transform data file to header file */
			if (fgStart) {
				/* disable first round flag */
				fgStart = 0;
				/* create output file and write corresponding power limit table */
				DataToHeaderTrans(infname, outfname, varname, deffname, "w");
			} else
				/* append corresponding power limit table to existed output file */
				DataToHeaderTrans(infname, outfname, varname, deffname, "a+");
		}
	}

	/* Power Limit Type String Free */
	PowerLimitTypeFree();
}
