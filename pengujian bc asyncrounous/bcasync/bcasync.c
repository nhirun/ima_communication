/*******************************************************************************
 *
 * Data Device Corporation
 * 105 Wilbur Place
 * Bohemia N.Y. 11716
 *
 * Filename: bcasync.c
 *
 * AceXtreme 'C' Run Time Library Toolbox
 *
 * Copyright (c) 2010 Data Device Corporation
 *
 *******************************************************************************
 * Non-Disclosure Statement
 * ------------------------
 * This software is the sole property of Data Device Corporation. All
 * rights, title, ownership, or other interests in the software remain
 * the property of Data Device Corporation. This software may be used
 * in accordance with applicable licenses. Any unauthorized use,
 * duplication, transmission, distribution, or disclosure is expressly
 * forbidden.
 *
 * This non-disclosure statement may not be removed or modified without
 * prior written consent of Data Device Corporation.
 *******************************************************************************
 * Module Description:
 *
 *   This application sends out three asynchronous messages in the low
 *   priority mode.  This means that messages will be stuffed into minor
 *   frames based on the avialablity of time.
 *
 *   Function used in this example:
 *
 *       aceInitialize
 *       aceFree
 *       aceGetLibVersion
 *       aceErrorStr
 *       aceGetMsgTypeString
 *       aceCmdWordParse
 *       aceGetBSWErrString
 *       aceBCGetHBufMsgDecoded
 *       aceBCGetHBufMsgRaw
 *       aceDataBlkCreate
 *       aceBCMsgCreateBCtoRT
 *       aceBCAsyncMsgCreateBCtoRT
 *       aceBCAsyncMsgCreateRTtoBC
 *       aceBCAsyncMsgCreateRTtoRT
 *       aceBCSendAsyncLP
 *       aceBCEmptyAsyncList
 *       aceBCOpCodeCreate
 *       aceBCFrameCreate
 *       aceBCInstallHBuf
 *       aceBCStart
 *       aceBCStop
 *
 *******************************************************************8**********/

/* include files */
#ifdef WIN32
#include <windows.h>
#include <conio.h>
#include "stdemace.h"
#endif

#ifdef LINUX
#include <emacepl/stdemace.h>
#include "../linuxutil.h"
#endif

#ifdef VX_WORKS
#include "../LibPrj/include/stdemace.h"
#include "../utils/vxwrksuti.h"
#endif

#ifdef INTEGRITY
#include "../Integrity_Samples.h"
#endif

/* define data blocks */
#define DBLK1   1
#define DBLK2   2
#define DBLK3   3
#define DBLK4   4
#define DBLK5   5

/* define message constants */
#define MSG1    1
#define MSG2    2
#define MSG3    3
#define MSG4    4
#define MSG5    5

/* define opcodes */
#define OP1     1
#define OP2     2

/* define frame constants */
#define MNR1    1
#define MJR     3

/* Change this to change your Frame Time */
#define wFrmTime    1000

/*******************************************************************************
 * Name:    PressAKey
 *
 * Description:
 *
 *      Allows application to pause to allow screen contents to be read.
 *
 * In   none
 * Out  none
 ******************************************************************************/
static void PressAKey()
{
    /* flush input buffer */
    while (kbhit())
    {
        getchar();
    }

    printf("\nPress <ENTER> to continue...");

    /* flush the output buffer */
    fflush(stdout);

    while (!kbhit())
    {
        /* wait for keypress... */
        Sleep(10);
    }

    /* flush input buffer */
    while (kbhit())
    {
        getchar();
    }
}

/*******************************************************************************
 * Name:    PrintHeader
 *
 * Description:
 *
 *      Prints the sample program header.
 *
 * In   none
 * Out  none
 ******************************************************************************/
static void PrintHeader()
{
    U16BIT wLibVer;

    wLibVer = aceGetLibVersion();

    printf("*********************************************\n");
    printf("AceXtreme Integrated 1553 Terminal          *\n");
    printf("BU-69092 1553 Runtime Library               *\n");
    printf("Release Rev %d.%d.%d                           *\n",
            (wLibVer>>8),        /* Major*/
            (wLibVer&0xF0)>>4,   /* Minor*/
            (wLibVer&0xF));      /* Devel*/

    if ((wLibVer&0xF)!=0)
        printf("=-=-=-=-=-=-=-INTERIM VERSION-=-=-=-=-=-=-=-*\n");

    printf("Copyright (c) 2010 Data Device Corporation  *\n");
    printf("*********************************************\n");
    printf("BCAsync.c BC operations [Asynchronous Demo] *\n");
    printf("*********************************************\n\n");

    printf(" This program wil continuously send messages to RT1/SA1,\n");
    printf(" with 10 data words: 0x0001 - 0x000A\n");
    printf("\n");
    printf(" Then 4 Asynchronous Messages will be sent out in Low Priority Mode:\n");
    printf("\n");
    printf("   BC --> RT2/SA1        5 Data Words\n");
    printf("   RT2/SA1 --> BC        10 Data Words\n");
    printf("   RT3/SA2 --> RT2/SA1   15 Data Words\n");
    printf("   BC --> RT2/SA1        20 Data Words\n");
}

/*******************************************************************************
 * Name:    PrintOutError
 *
 * Description:
 *
 *      This function prints out errors returned from library functions.
 *
 * In   result - the error number
 * Out  none
 ******************************************************************************/
static void PrintOutError(S16BIT nResult)
{
    char buf[80];
    aceErrorStr(nResult,buf,80);
    printf("RTL Function Failure-> %s.\n",buf);
}


/*******************************************************************************
 * Name:    OutputDecodedMsg
 *
 * Description:
 *
 *      This function outputs messages to a BC stack file in ASCII text.
 *
 * In   nMsgNum     Message sequence number
 * In   MSGSTRUCT   The message
 * In   pFileName   Pointer to a null terminated string containing file name
 * Out  none
 ******************************************************************************/
static void OutputDecodedMsg(U32BIT nMsgNum,MSGSTRUCT *pMsg,FILE *pFile)
{
    U16BIT i;
    char szBuffer[100];
    U16BIT wRT,wTR1,wTR2,wSA,wWC;

    /* Display message header info */
    fprintf(pFile,"MSG #%04d.  TIME = %08dus    BUS %c   TYPE%d: %s ",
       (int)nMsgNum,pMsg->wTimeTag*2,pMsg->wBlkSts&ACE_BC_BSW_CHNL?'B':'A',
       pMsg->wType, aceGetMsgTypeString(pMsg->wType));

    /* Display command word info */
    aceCmdWordParse(pMsg->wCmdWrd1,&wRT,&wTR1,&wSA,&wWC);
    sprintf(szBuffer,"%02d-%c-%02d-%02d",wRT,wTR1?'T':'R',wSA,wWC);
    fprintf(pFile,"\n            CMD1 %04X --> %s",pMsg->wCmdWrd1, szBuffer);

    if(pMsg->wCmdWrd2Flg)
    {
        aceCmdWordParse(pMsg->wCmdWrd2,&wRT,&wTR2,&wSA,&wWC);
        sprintf(szBuffer,"%02d-%c-%02d-%02d",wRT,wTR2?'T':'R',wSA,wWC);
        fprintf(pFile,"\n            CMD2 %04X --> %s",pMsg->wCmdWrd2, szBuffer);
    }

    /* Display transmit status words */
    if((wTR1==1) || (pMsg->wBCCtrlWrd&ACE_BCCTRL_RT_TO_RT))
    {
        if(pMsg->wStsWrd1Flg)
        fprintf(pFile,"\n            STA1 %04X",pMsg->wStsWrd1);
    }

    /* Display Data words */
    for(i=0; i<pMsg->wWordCount; i++)
    {
        if(i==0)
            fprintf(pFile,"\n            DATA ");

        fprintf(pFile,"%04X  ",pMsg->aDataWrds[i]);

        if((i%8)==7)
            fprintf(pFile,"\n                 ");
    }

    /* Display receive status words */
    if((wTR1==0)  && !(pMsg->wBCCtrlWrd&ACE_BCCTRL_RT_TO_RT))
    {
        if(pMsg->wStsWrd1Flg)
        fprintf(pFile,"\n            STA1 %04X",pMsg->wStsWrd1);
    }

    if(pMsg->wStsWrd2Flg)
    {
       fprintf(pFile,"\n            STA2 %04X",pMsg->wStsWrd2);
    }

    /* Display Error information */
    if(pMsg->wBlkSts & 0x170f)
    {
       fprintf(pFile, "\n ERROR: %s",aceGetBSWErrString(ACE_MODE_BC,pMsg->wBlkSts));
    }

    fprintf(pFile,"\n---------------------------------------------------------\n");
}

/*******************************************************************************
 * Name:    DisplayDecodedMsg
 *
 * Description:
 *      This function displays the information of a MSGSTRUCT to the screen.
 *
 * In   nMsgNum     Number of the message
 * In   pMsg        Relavant information of the message
 * Out  none
 ******************************************************************************/
static void DisplayDecodedMsg(S16BIT nMsgNum,MSGSTRUCT *pMsg)
{
    U16BIT i;
    char szBuffer[100];
    U16BIT wRT,wTR1,wTR2,wSA,wWC;

    /* Display message header info */
    printf("MSG #%04d.  TIME = %08dus    BUS %c   TYPE%d: %s ",nMsgNum,
        pMsg->wTimeTag*2,pMsg->wBlkSts&ACE_MT_BSW_CHNL?'B':'A',
        pMsg->wType, aceGetMsgTypeString(pMsg->wType));

    /* Display command word info */
    aceCmdWordParse(pMsg->wCmdWrd1,&wRT,&wTR1,&wSA,&wWC);
    sprintf(szBuffer,"%02d-%c-%02d-%02d",wRT,wTR1?'T':'R',wSA,wWC);
    printf("\n            CMD1 %04X --> %s",pMsg->wCmdWrd1,szBuffer);

    if(pMsg->wCmdWrd2Flg)
    {
        aceCmdWordParse(pMsg->wCmdWrd2,&wRT,&wTR2,&wSA,&wWC);
        sprintf(szBuffer,"%02d-%c-%02d-%02d",wRT,wTR2?'T':'R',wSA,wWC);
        printf("\n        CMD2 %04X --> %s",pMsg->wCmdWrd2,szBuffer);
    }

    /* Display transmit status words */
    if((wTR1==1) || (pMsg->wBlkSts&ACE_MT_BSW_RTRT))
    {
        if(pMsg->wStsWrd1Flg)
            printf("\n            STA1 %04X",pMsg->wStsWrd1);
    }

    /* Display Data words */
    for(i=0; i<pMsg->wWordCount; i++)
    {
        if(i==0)
            printf("\n            DATA ");

       printf("%04X  ",pMsg->aDataWrds[i]);

        if((i%8)==7)
            printf("\n                 ");
    }

    /* Display receive status words */
    if((wTR1==0)  && !(pMsg->wBlkSts&ACE_MT_BSW_RTRT))
    {
        if(pMsg->wStsWrd1Flg)
            printf("\n            STA1 %04X",pMsg->wStsWrd1);
    }

    if(pMsg->wStsWrd2Flg)
       printf("\n            STA2 %04X",pMsg->wStsWrd2);

    /* Display Error information */
    if(pMsg->wBlkSts & ACE_MT_BSW_ERRFLG)
       printf("\n ERROR: %s",aceGetBSWErrString(ACE_MODE_MT,pMsg->wBlkSts));

    printf("\n\n");
}

/*******************************************************************************
 * Name:    GetBCHBufRawMsgs
 *
 * Description:
 *
 *      This function creates an BC stack file in ASCII text using all msgs
 *      read from the ACE and outputs to a file. Sendsout asynch messages once.
 *
 * In   DevNum - the device number of the hardware to be accessed
 * Out  none
 ******************************************************************************/
static void GetBCHBufRawMsgs(S16BIT DevNum)
{
    /* Variables */
#ifdef INTEGRITY
    Task kbhit_task;
#else
    FILE *pFile = NULL;
#endif
    U16BIT wMsgCount    = 0x0000;
    U32BIT dwMsgCount   = 0x00000000;
    U32BIT dwMsgLost    = 0x00000000;
    U32BIT dwPrevCount  = 0x00000000;
    U32BIT dwCurCount   = 0x00000000;
    S16BIT nResult      = 0x0000;
    U16BIT wMsgLeft     = 0;
    MSGSTRUCT sMsg;

    U16BIT wBuffer2[64] =
    {
        0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
        0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
        0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40
    };

#ifndef INTEGRITY
    /* Open file */
    pFile = fopen("bcstack.txt","w+");
#endif

    /* Start BC */
    aceBCStart(DevNum,MJR,-1);
    printf("\n\n");

    /* Run Information */
    printf("\n** BC Device Number %d, outputting to 'bcstack.txt', press <ENTER> to stop **\n\n", DevNum);

    PressAKey();

    printf("\n");

    /* Start sending asynchronous messages */
    aceBCSendAsyncMsgLP(DevNum,&wMsgLeft,wFrmTime);

    /* Create Asynchronous Op Code */
    aceBCAsyncMsgCreateBCtoRT(
        DevNum,             /* Device number                    */
        MSG5,               /* Message ID to create             */
        DBLK5,              /* Data Block ID                    */
        2,                  /* RT address                       */
        1,                  /* RT subaddress                    */
        20,                 /* Word Count                       */
        0,                  /* Devault Message Timer            */
        ACE_BCCTRL_CHL_A,   /* use chl A options                */
        wBuffer2);          /* Data words for the async message */

    wMsgLeft++;

#ifdef INTEGRITY
    CreateProtectedTask(1,(Address)kbhit_task_func,0x1000,"kbhit", &kbhit_task);
    RunTask(kbhit_task);
#endif

    while(!kbhit())
    {
        /* Check host buffer for msgs */
        nResult = aceBCGetHBufMsgDecoded(DevNum,&sMsg,&dwMsgCount, &dwMsgLost,ACE_BC_MSGLOC_NEXT_PURGE);

        if(nResult)
        {
            printf("Error reading host buffer\n");
        }

        /* Message found */
        if(dwMsgCount)
        {
            dwPrevCount += dwMsgLost;
            dwCurCount += dwMsgCount;

            /* Display msg lost count */
            if(dwMsgLost > 0)
                printf(" Number of msgs lost %u\n",(int)dwMsgLost);

            wMsgCount++;

#ifndef INTEGRITY
            OutputDecodedMsg(wMsgCount,&sMsg,pFile);
#endif

            DisplayDecodedMsg(wMsgCount,&sMsg);
        }

        if(wMsgLeft!=0)
        {
            aceBCSendAsyncMsgLP(DevNum,&wMsgLeft,wFrmTime);
        }

        Sleep(1);
    }

    printf("BC: Total msgs captured: %d                  \n", (int)(dwCurCount+dwMsgLost));

    /* Empty list of asynchronous messsages */
    aceBCEmptyAsyncList(DevNum);

    /* Throw away key */
    getch();

    /* Stop BC */
    aceBCStop(DevNum);

    printf("\n");
}

/*******************************************************************************
 * Name:    GetBCHBufRawMsgs2
 *
 * Description:
 *
 *      This function creates an BC stack file in ASCII text using all msgs
 *      read from the ACE and outputs to a file. Sends out asynch messages
 *      more than once.
 *
 * In   DevNum - the device number of the hardware to be accessed
 * Out  none
 ******************************************************************************/
static void GetBCHBufRawMsgs2(S16BIT DevNum)
{
   /* Variables */
#ifdef INTEGRITY
    Task kbhit_task;
#else
    FILE *pFile = NULL;
#endif

    U32BIT dwMsgCount   = 0x00000000;
    U32BIT dwMsgLost    = 0x00000000;
    U32BIT dwPrevCount  = 0x00000000;
    U32BIT dwCurCount   = 0x00000000;
    U16BIT wMsgLeft     = 0;
    U16BIT bResend      = FALSE;
    S16BIT nResult      = 0x0000;
    S16BIT i            = 0x0000;

    U16BIT wBuffer[64] =
    {
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    };

    U16BIT wBuffer2[64] =
    {
        0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
        0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
        0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40
    };

#ifndef INTEGRITY
    /* Open file */
    pFile = fopen("bcstack.bin","wb+");
#endif

    /* Start BC */
    aceBCStart(DevNum,MJR,-1);

    /* Run Information */
    printf("\n** BC Device Number %d, outputting to 'bcstack.bin', press <ENTER> to stop **\n\n", DevNum);

    PressAKey();

    printf("\n");

    /* Start sending asynchronous messages */
    aceBCSendAsyncMsgLP(DevNum,&wMsgLeft,wFrmTime);

    /* Create Asynchronous Op Code */
    aceBCAsyncMsgCreateBCtoRT(
        DevNum,             /* Device number                    */
        MSG5,               /* Message ID to create             */
        DBLK5,              /* Data Block ID                    */
        2,                  /* RT address                       */
        1,                  /* RT subaddress                    */
        20,                 /* Word Count                       */
        0,                  /* Devault Message Timer            */
        ACE_BCCTRL_CHL_A,   /* use chl A options                */
        wBuffer2);          /* Data words for the async message */

    wMsgLeft++;

#ifdef INTEGRITY
    CreateProtectedTask(1,(Address)kbhit_task_func,0x1000,"kbhit", &kbhit_task);
    RunTask(kbhit_task);
#endif

    while(!kbhit())
    {
        /* Check host buffer for msgs */
        nResult = aceBCGetHBufMsgsRaw(DevNum,wBuffer,ACE_MSGSIZE_BC, &dwMsgCount,&dwMsgLost);

        /* Msg found */
        if(nResult)
        {
            printf("Error reading host buffer\n");
        }

        /* Msg found */
        if(dwMsgCount)
        {
            dwPrevCount += dwMsgLost;
            dwCurCount  += dwMsgCount;

            for(i=0; dwPrevCount<dwCurCount; dwPrevCount++,i++)
            {
#ifndef INTEGRITY
                fwrite(&wBuffer[i*ACE_MSGSIZE_BC], 84, 1, pFile);
#endif
            }

            printf("BC: Total msgs captured: %d                \r", (int)(dwCurCount+dwMsgLost));
        }
        else
        {
            if(bResend == FALSE)
            {
                aceBCSendAsyncMsgLP(DevNum,&wMsgLeft,wFrmTime);

                if(wMsgLeft == 0)
                {
                    bResend = TRUE;
                }
            }
            else if((wMsgLeft == 0) && (bResend == TRUE))
            {
                aceBCResetAsyncPtr(DevNum);
                bResend = FALSE;
            }

            Sleep(1);
            continue;
        }

        if(bResend == FALSE)
        {
            aceBCSendAsyncMsgLP(DevNum,&wMsgLeft,wFrmTime);

            if(wMsgLeft == 0)
            {
                bResend = TRUE;
            }
        }
        else if((wMsgLeft == 0) && (bResend == TRUE))
        {
            aceBCResetAsyncPtr(DevNum);
            bResend = FALSE;
        }

        Sleep(1);
    }

    /* Empty list of asynchronous messsages */
    aceBCEmptyAsyncList(DevNum);

    /* Throw away key */
    getch();

    /* Stop BC */
    aceBCStop(DevNum);

    printf("\n");
}

/*******************************************************************************
 * Name:    main
 *
 * Description:
 *
 *      Program entry point.
 *
 * In             none
 * Out            none
 ******************************************************************************/
#ifndef VX_WORKS
int main(void)
#else
int bcasync(void)
#endif
{
    /* Variables */
    S16BIT DevNum       = 0x0000;
    S16BIT wResult      = 0x0000;
    S16BIT wSelection   = 0x0000;
    S16BIT aOpCodes[10] = { 0x0000 };

    U16BIT wBuffer[64] =
    {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };

    U16BIT wBuffer2[64] =
    {
        0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
        0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
        0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40
    };


    U16BIT wBuffer3[64] =
    {
        0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
        0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50,
        0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
        0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60
    };

    U16BIT wBuffer4[64] =
    {
        0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
        0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70,
        0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
        0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80
    };

#ifdef WIN32
    HANDLE hConsole = NULL;

    /* Setup Windows Console Screen */
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTitle("DDC Enhanced Mini-ACE RTL [BCAsync.c - Console App BC (Basic Operations) Demo]");
#endif

    /* Print out sample header */
    PrintHeader();

    /* Get Logical Device # */
    printf("\nSelect BC Logical Device Number (0-31):\n");
    printf("> ");
    scanf("%hd", &DevNum);

    /* Initialize Device */
    wResult = aceInitialize(DevNum,ACE_ACCESS_CARD,ACE_MODE_BC,0,0,0);
    if(wResult)
    {
        printf("Initialize ");
        PrintOutError(wResult);
        PressAKey();
        return 0;
    }

    /* Enable low priority asynchronous mode */
    wResult = aceBCConfigure(DevNum,ACE_BC_ASYNC_LMODE);

    if(wResult)
    {
        printf("Configure ");
        PrintOutError(wResult);
        PressAKey();
        return 0;
   }

    /* Create 3 data blocks */
    aceBCDataBlkCreate((S16BIT)DevNum,DBLK1,32,wBuffer,32);

    /* Create message block */
    aceBCMsgCreateBCtoRT(
        DevNum,             /* Device number                    */
        MSG1,               /* Message ID to create             */
        DBLK1,              /* Message will use this data block */
        1,                  /* RT address                       */
        1,                  /* RT subaddress                    */
        10,                 /* Word count                       */
        0,                  /* Default message timer            */
        ACE_BCCTRL_CHL_A);  /* use chl A options                */

    /* Create XEQ opcode that will use msg block */
    aceBCOpCodeCreate(DevNum,OP1,ACE_OPCODE_XEQ,ACE_CNDTST_ALWAYS,MSG1,0,0);

    /* Create CAL opcode that will call mnr frame from major */
    aceBCOpCodeCreate(DevNum,OP2,ACE_OPCODE_CAL,ACE_CNDTST_ALWAYS,MNR1,0,0);

    /* Create Minor Frame */
    aOpCodes[0] = OP1;
    aceBCFrameCreate(DevNum,MNR1,ACE_FRAME_MINOR,aOpCodes,1,0,0);

    /* Create Major Frame */
    aOpCodes[0] = OP2;
    aceBCFrameCreate(DevNum,MJR,ACE_FRAME_MAJOR,aOpCodes,1,wFrmTime,0);

    /* Create Asynchronous Msg */
    aceBCAsyncMsgCreateBCtoRT(
        DevNum,             /* Device number                    */
        MSG2,               /* Message ID to create             */
        DBLK2,              /* Data Block ID                    */
        2,                  /* RT address                       */
        1,                  /* RT subaddress                    */
        5,                  /* Word Count                       */
        0,                  /* Devault Message Timer            */
        ACE_BCCTRL_CHL_A,   /* use chl A options                */
        wBuffer2);          /* Data words for the async message */

    /* Create Asynchronous Msg */
    aceBCAsyncMsgCreateRTtoBC(
        DevNum,             /* Device number                    */
        MSG3,               /* Message ID to create             */
        DBLK3,              /* Data Block ID                    */
        2,                  /* RT address                       */
        1,                  /* RT subaddress                    */
        10,                 /* Word Count                       */
        0,                  /* Devault Message Timer            */
        ACE_BCCTRL_CHL_A,   /* use chl A options                */
        wBuffer3);          /* Data words for the async message */

    /* Create Asynchronous Msg */
    aceBCAsyncMsgCreateRTtoRT(
        DevNum,             /* Device number                    */
        MSG4,               /* Message ID to create             */
        DBLK4,              /* Data Block ID                    */
        2,                  /* RT address                       */
        1,                  /* RT subaddress                    */
        15,                 /* Word Count                       */
        3,
        2,
        0,                  /* Default Message Timer            */
        ACE_BCCTRL_CHL_A,   /* use chl A options                */
        wBuffer4);          /* Data words for the async message */

    /* Create Host Buffer */
    aceBCInstallHBuf(DevNum,32*1024);

    printf("\nMake Selection: \n");
    printf("1. Send Async messages once\n");
    printf("2. Send Async messages repeatedly\n");
    printf ("> ");
    scanf("%hd", &wSelection);

#ifdef INTEGRITY
        /* Get kbhit timeout */
    printf("\nHow long would you like this demo to run for? (in minutes) :\n");
    printf("> ");
    scanf("%hd", &kbhit_length);
#endif

    switch(wSelection)
    {
        case 1:  GetBCHBufRawMsgs(DevNum);  break;
        case 2:  GetBCHBufRawMsgs2(DevNum); break;
        default: GetBCHBufRawMsgs(DevNum);  break;
    }

    /* Free all resources */
    wResult = aceFree(DevNum);

    if(wResult)
    {
        printf("Free ");
        PrintOutError(wResult);
        return 0;
    }

    /* Pause before program exit */
    PressAKey();

    return 0;
}

