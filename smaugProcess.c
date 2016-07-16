#include <errno.h>
#include <wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <curses.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/resource.h>


/* Define semaphores to be placed in a single semaphore set */
/* Numbers indicate index in semaphore set for named semaphore */
#define SEM_COWSINGROUP 0
#define SEM_PCOWSINGROUP 1
#define SEM_PSHEEPINGROUP 2
#define SEM_SHEEPINGROUP 3
#define SEM_SHEEPWAITING 4
#define SEM_COWSWAITING 5
#define SEM_PSHEEPEATEN 6
#define SEM_PCOWSEATEN 7
#define SEM_SHEEPEATEN 8
#define SEM_COWSEATEN 9
#define SEM_SHEEPDEAD 10
#define SEM_COWSDEAD 11
#define SEM_PTERMINATE 12
#define SEM_DRAGONEATING 13
#define SEM_DRAGONFIGHTING 14
#define SEM_DRAGONSLEEPING 15
#define SEM_DRAGONTREASURE 16
#define SEM_HUNTERSLEFT 17
#define SEM_HUNTERSFOUGHT 18
#define SEM_HUNTERSWAITING 19
#define SEM_PHUNTERSFOUGHT 20
#define SEM_THIEVESLEFT 21
#define SEM_THIEVESWAITING 22
#define SEM_PTHIEVESFOUGHT 23
#define SEM_THIEVESFOUGHT 24
#define SEM_PMEALWAITINGFLAG 25

/* System constants used to control simulation termination */
#define MAX_COWS_EATEN 14
#define MAX_SHEEP_EATEN 14
#define MAX_HUNTERS_DEFEATED 12
#define MAX_THIEVES_DEFEATED 15
#define MAX_COWS_CREATED 80
#define MAX_TREASURE_IN_HOARD 800
#define INITIAL_TREASURE_IN_HOARD 400


/* System constants to specify size of groups of cows*/
#define COWS_IN_GROUP 2
#define SHEEPS_IN_GROUP 2

/* CREATING YOUR SEMAPHORES */
int semID;

union semun
{
	int val;
	struct semid_ds *buf;
	ushort *array;
} seminfo;

struct timeval startTime;


/*  Pointers and ids for shared memory segments */
int *terminateFlagp = NULL;
int *sheepCounterp = NULL;
int *Prob_Winp = NULL;
int sheepCounter = 0;
int *sheepEatenCounterp = NULL;
int sheepEatenCounter = 0;
int *cowCounterp = NULL;
int *cowsEatenCounterp = NULL;
int cowMealFlag = 0;
int cowsEatenCounter = 0;
int *hunterCounterp = NULL;
int hunterCounter = 0
int *hunterFoughtCounterp = NULL;
int huntersFoughtCounterp = 0;
int *thiefCounterp = NULL;
int *thiefWaitingFlagp = NULL;
int *hunterWaitingFlagp = NULL;
int thiefCounter = 0;
int *thievesFoughtCounterp = NULL;
int thievesFoughtCounter = 0;
int *mealWaitingFlagp = NULL;
int terminateFlag = 0;
int cowCounter = 0;
int cowsEatenCounter = 0;
int mealWaitingFlag = 0;

/* Group IDs for managing/removing processes */
int smaugProcessID = -1;
int cowProcessGID = -1;
int parentProcessGID = -1;
int sheepProcessGID = -1;
int thiefProcessGID = -1;
int hunterProcessGID = -1;

/* Define the semaphore operations for each semaphore */
/* Arguments of each definition are: */
/* Name of semaphore on which the operation is done */
/* Increment (amount added to the semaphore when operation executes*/
/* Flag values (block when semaphore <0, enable undo ...)*/

/*Number in group semaphores*/
struct sembuf WaitCowsInGroup={SEM_COWSINGROUP, -1, 0};
struct sembuf SignalCowsInGroup={SEM_COWSINGROUP, 1, 0};
struct sembuf WaitSheepInGroup = {SEM_SHEEPINGROUP, -1, 0};
struct sembuf SignalSheepInGroup = {SEM_SHEEPINGROUP, 1, 0};

/*Number in group mutexes*/
struct sembuf WaitProtectCowsInGroup={SEM_PCOWSINGROUP, -1, 0};
struct sembuf WaitProtectMealWaitingFlag={SEM_PMEALWAITINGFLAG, -1, 0};
struct sembuf WaitProtectSheepInGroup={SEM_PSHEEPINGROUP, -1, 0};
struct sembuf SignalProtectCowsInGroup={SEM_PCOWSINGROUP, 1, 0};
struct sembuf SignalProtectSheepInGroup={SEM_PSHEEPINGROUP, 1, 0};
struct sembuf SignalProtectMealWaitingFlag={SEM_PMEALWAITINGFLAG, 1, 0};

/*Number waiting sempahores*/
struct sembuf WaitCowsWaiting={SEM_COWSWAITING, -1, 0};
struct sembuf SignalCowsWaiting={SEM_COWSWAITING, 1, 0};
struct sembuf WaitSheepWaiting = {SEM_SHEEPWAITING, -1, 0};
struct sembuf SignalSheepWaiting = {SEM_SHEEPWAITING, 1, 0};
struct sembuf WaitThievesWaiting = {SEM_THIEVESWAITING, -1, 0};
struct sembuf SignalThievesWaiting = {SEM_THEIVESWAITING, 1, 0};
struct sembuf WaitHuntesrWaiting = {SEM_HUNTERSWAITING, -1, 0};
struct sembuf SignalHuntersWaiting = {SEM_HUNTERSWAITING, 1, 0};

/*Number eaten or fought semaphores*/
struct sembuf WaitCowsEaten={SEM_COWSEATEN, -1, 0};
struct sembuf SignalCowsEaten={SEM_COWSEATEN, 1, 0};
struct sembuf WaitSheepEaten = {SEM_SHEEPEATEN, -1, 0};
struct sembuf SignalSheepEaten = {SEM_SHEEPEATEN, 1, 0};
struct sembuf WaitTheivesFought = {SEM_THEIVESFOUGHT, -1, 0};
struct sembuf SignalThievesFought = {SEM_THEIVESFOUGHT, 1, 0};
struct sembuf WaitHuntersFought = {SEM_HUNTERSFOUGHT, -1, 0};
struct sembuf SignalHuntersFought = {SEM_HUNTERSFOUGHT, 1, 0};

/*Number eaten or fought mutexes*/
struct sembuf WaitProtectCowsEaten={SEM_PCOWSEATEN, -1, 0};
struct sembuf SignalProtectCowsEaten={SEM_PCOWSEATEN, 1, 0};
struct sembuf WaitProtectSheepEaten = {SEM_PSHEEPEATEN, -1, 0};
struct sembuf SignalProtectSheepEaten = {SEM_PSHEEPEATEN, 1, 0};
struct sembuf WaitProtectThievesFought = {SEM_PTHEIVESFOUGHT, -1, 0};
struct sembuf SignalProtectThievesFought = {SEM_PTHIEVESFOUGHT, 1, 0};
struct sembuf WaitProtectHuntersFought = {SEM_PHUNTERSFOUGHT, -1, 0};
struct sembuf SignalProtectHuntersFought = {SEM_PHUNTERSFOUGHT, 1, 0};
struct sembuf WaitProtectHuntersWaitingFlag={SEM_PHUNTERSWAITINGFLAG, -1, 0};
struct sembuf SignalProtectHuntersWaitingFlag={SEM_PHUNTERSWAITINGFLAG, 1, 0};
struct sembuf WaitProtectThiefWaitingFlag={SEM_PTHIEFWAITINGFLAG, -1,0};
struct sembuf SignalProtectThiefWaitingFlag={SEM_PTHIEFWAITINGFLAG, 1,0};
struct sembuf WaitProtectFightResults={SEM_PFIGHTRESULTS, -1, 0};
struct sembuf SignalProtectFightResults={SEM_PFIGHTRESULTS, 1, 0};

/*Number Dead semaphores*/
struct sembuf WaitCowsDead={SEM_COWSDEAD, -1, 0};
struct sembuf SignalCowsDead={SEM_COWSDEAD, 1, 0};
struct sembuf WaitSheepDead={SEM_SHEEPDEAD, -1, 0};
struct sembuf SignalSheepDead = {SEM_SHEEPDEAD, 1, 0);

/*Dragon Semaphores*/
struct sembuf WaitDragonEating={SEM_DRAGONEATING, -1, 0};
struct sembuf SignalDragonEating={SEM_DRAGONEATING, 1, 0};
struct sembuf WaitDragonFighting={SEM_DRAGONFIGHTING, -1, 0};
struct sembuf SignalDragonFighting={SEM_DRAGONFIGHTING, 1, 0};
struct sembuf WaitDragonSleeping={SEM_DRAGONSLEEPING, -1, 0};
struct sembuf SignalDragonSleeping={SEM_DRAGONSLEEPING, 1, 0};
struct sembuf WaitFightEnded = {SEM_FIGHTENDED, -1, 0};
struct sembuf SignalFightEnded = {SEM_FIGHTENDED, 1, 0};
struct sembuf WaitPlayEnded = {SEM_PLAYENDED, -1, 0};
struct sembuf SignalPlayEnded = {SEM_PLAYENDED, 1, 0};

/*Termination Mutex*/
struct sembuf WaitProtectTerminate={SEM_PTERMINATE, -1, 0};
struct sembuf SignalProtectTerminate={SEM_PTERMINATE, 1, 0};


double timeChange( struct timeval starttime );
void initialize();
void smaug();
void cow(int startTimeN);
void sheep(int startTimeN);
void thief(int startTimeN);
void hunter(int startTimeN);
void terminateSimulation();
void releaseSemandMem();
void semopChecked(int semaphoreID, struct sembuf *operation, unsigned something);
void semctlChecked(int semaphoreID, int semNum, int flag, union semun seminfo);

int main( )
{
	int k;
	int temp;

	/* variables to hold process ID numbers */
	int parentPID = 0;
	int cowPID = 0;
	int sheepPID = 0;
	int smaugPID = 0;
	int hunterPID = 0;
	int thiefPID = 0;

	/* local counters, keep track of total number */
	/* of processes of each type created */
	int cowsCreated = 0;
	int huntersCreated = 0;
	int thievesCreated = 0;

	/* Variables to control the time between the arrivals */
	/* of successive beasts*/
	double minwait = 0;
	int newSeed = 0;
	int sleepingTime = 0;
	int maxCowIntervalUsec = 0;
	int maxSheepIntervalUsec = 0;
	int maxHunterIntervalUsec = 0;
	int maxThiefIntervalUsec = 0;
	int nextInterval = 0.0;
	int status;
	int w = 0;
	int winProb2 = 0;
	double maxCowInterval = 0.0;
	double maxSheepInterval = 0.0;
	double maxHunterInterval = 0.0;
	double maxThiefInterval = 0.0;
	double totalCowInterval = 0.0;
	double totalSheepInterval = 0.0;
	double totalThiefInterval = 0.0;
	double totalHunterInterval = 0.0;
	double elapsedTime;
	double hold;

	parentPID = getpid();
	setpgid(parentPID, parentPID);
	parentProcessGID = getpgid(0);
	printf("CRCRCRCRCRCRCRCRCRCRCRCR  main process group  %d %d\n", parentPID, parentProcessGID);

	/* initialize semaphores and allocate shared memory */
	initialize();

	/* inialize each variable in shared memory */
	*cowCounterp = 0;
	*cowsEatenCounterp = 0;
	*mealWaitingFlagp = 0;

	printf("Please enter a random seed to start the simulation\n");
	scanf("%d",&newSeed);
	srand(newSeed);

	printf("Please enter the maximum interval length for sheep (ms) ");
	scanf("%lf", &maxCowInterval);
	maxCowIntervalUsec = (int)maxCowInterval * 1000;
	printf("max Sheep interval time %f \n", maxCowInterval);

	printf("Please enter the maximum interval length for cow (ms) ");
	scanf("%lf", &maxSheepInterval);
	maxSheepIntervalUsec = (int)maxSheepInterval * 1000;
	printf("max cow interval time %f \n", maxSheepInterval);

	printf("Please enter the maximum interval length for Hunter (ms) ");
	scanf("%lf", &maxHunterInterval);
	maxHunterIntervalUsec = (int)maxHunterInterval * 1000;
	printf("max Hunter interval time %f \n", maxHunterInterval);

	printf("Please enter the maximum interval length for Thief (ms) ");
	scanf("%lf", &maxThiefInterval);
	maxThiefIntervalUsec = (int)maxThiefInterval * 1000;
	printf("max thief interval time %f \n", maxThiefInterval);

	printf("Please enter the win probability for visitors ");
	scanf("%d", &Prob_Winp2);
	*Prob_Winp = Prob_Winp2;

	gettimeofday(&startTime,NULL);

	if ( ( smaugPID = fork())==0) {
		printf("CRCRCRCRCRCRCRCRCRCRCRCR  Smaug is born\n");
		smaug();
		printf("CRCRCRCRCRCRCRCRCRCRCRCR  Smaug dies\n");
		exit(0);
	}
	else {
		if(smaugProcessID == -1) smaugProcessID = smaugPID;
		setpgid(smaugPID, smaugProcessID);
		printf("CRCRCRCRCRCRCRCRCRCRCRCR  Smaug PID %8d PGID %8d\n", smaugPID, smaugProcessID);
	}

	printf("CRCRCRCRCRCRCRCRCRCRCRCR  Smaug PID  create cow %8d \n", smaugPID);
      usleep(10);
    while( 1 ) {

		semopChecked(semID, &WaitProtectTerminate, 1);
       		if( *terminateFlagp != 0)  {
			semopChecked(semID, &SignalProtectTerminate, 1);
			break;
		}
		semopChecked(semID, &SignalProtectTerminate, 1);

		elapsedTime = timeChange(startTime);
		if( totalCowInterval - elapsedTime < totalCowInterval) {
			nextInterval =(int)((double)rand() / RAND_MAX *  maxCowIntervalUsec);
			totalCowInterval += nextInterval/1000.0;
			sleepingTime = (int)( (double)rand()/ RAND_MAX *  maxCowIntervalUsec);
			if ( (cowPID = fork())==0) {
				elapsedTime = timeChange(startTime);
				cow( sleepingTime );
				exit(0);
			}
			else if( cowPID > 0) {
				cowsCreated++;
				if(cowProcessGID == -1){
					cowProcessGID = cowPID;
					printf("CRCRCRCRCR %8d  CRCRCRCRCR  sheep PGID %8d \n", cowPID, cowProcessGID);
				}
				setpgid(cowPID, cowProcessGID);
				printf("CRCRCRCRCRCRCRCRCRCRCRCR   NEW SHEEP CREATED %8d \n", cowsCreated);
			}
			else {
				printf("CRCRCRCRCRCRCRCRCRCRCRCRcow process not created \n");
				continue;
			}
			/*sleep for 80% of the cow interval*/
			/*then try making more processes */
	        usleep( (totalCowInterval*800) );

		}
		elapsedTime = timeChange(startTime);
		if( totalSheepInterval - elapsedTime < totalSheepInterval) {
			nextInterval =(int)((double)rand() / RAND_MAX *  maxSheepIntervalUsec);
			totalSheepInterval += nextInterval/1000.0;
			sleepingTime = (int)( (double)rand()/ RAND_MAX *  maxSheepIntervalUsec);
			if ( (sheepPID = fork())==0) {
				/* Child becomes a beast */
				elapsedTime = timeChange(startTime);
				sheep( sleepingTime );
				/* Child (beast) quits after being consumed */
				exit(0);
			}
			else if( sheepPID > 0) {
				if(sheepProcessGID == -1){
					sheepProcessGID = sheepPID;
					printf("CRCRCRCRCR %8d  CRCRCRCRCR  cow PGID %8d \n", sheepPID, sheepProcessGID);
				}
				setpgid(sheepPID, sheepProcessGID);
				printf("CRCRCRCRCRCRCRCRCRCRCRCR   NEW COW CREATED %8d \n", cowsCreated);
			}
			else {
				printf("CRCRCRCRCRCRCRCRCRCRCRCRcow process not created \n");
				continue;
			}
			/*sleep for 80% of the sheep interval*/
			/*then try making more processes */
	        usleep( (totalSheepInterval*800) );
		}

		elapsedTime = timeChange(startTime);
	 	if(totalHunterInterval - elapsedTime < totalHunterInterval){
			nextInterval = (int)((double)rand() / RAND_MAX * maxHunterIntervalUsec);
			totalHunterInterval += nextInterval/1000.0;
			sleepingTime = (int)( (double)rand()/ RAND_MAX *  maxHunterIntervalUsec);

			if((hunterPID = fork()) == 0){
				elapsedTime = timeChange(startTime);
				hunter(sleepingTime);
				exit(0);
			}
			else if(hunterPID > 0){
				huntersCreated++;
				if(hunterProcessGID == -1){
					hunterProcessGID = hunterPID;
					printf("CRCRCRCRCR %8d  CRCRCRCRCR  Hunter PGID %8d \n", hunterPID, hunterProcessGID);
				}
				setpgid(hunterPID, hunterProcessGID);
				printf("CRCRCRCRCRCRCRCRCRCRCRCR   NEW HUNTER CREATED %8d \n", huntersCreated);
			}
			else{
				printf("CRCRCRCRCRCRCRCRCRCRCRCRcow process not created \n");
				continue;
			}
			if( *defeatedHuntersCountp >= MAX_HUNTERS_DEFEATED ) {
				printf("CRCRCRCRCRCRCRCRCRCRCRCR Exceeded maximum number of defeated hunters\n");
				*terminateFlagp = 1;
				break;
			}
			usleep( (totalHunterInterval*800) );
		}
		elapsedTime = timeChange(startTime);
	 	if(totalThiefInterval - elapsedTime < totalThiefInterval){
			nextInterval = (int)((double)rand() / RAND_MAX * maxThiefIntervalUsec);
			totalThiefInterval += nextInterval/1000.0;
			sleepingTime = (int)( (double)rand()/ RAND_MAX *  maxThiefIntervalUsec);

			if((thiefPID = fork()) == 0){
				elapsedTime = timeChange(startTime);
				thief(sleepingTime);
				exit(0);
			}
			else if(thiefPID > 0){
				thievesCreated++;
				if(thiefProcessGID == -1){
					thiefProcessGID = thiefPID;
					printf("CRCRCRCRCR %8d  CRCRCRCRCR  thief PGID %8d \n", thiefPID, thiefProcessGID);
				}
				setpgid(thiefPID, thiefProcessGID);
				printf("CRCRCRCRCRCRCRCRCRCRCRCR   NEW THIEF CREATED %8d \n", thievesCreated);
			}
			else{
				printf("CRCRCRCRCRCRCRCRCRCRCRCRcow process not created \n");
				continue;
			}
			if( *defeatedThievesCountp >= MAX_THIEVES_DEFEATED ) {
				printf("CRCRCRCRCRCRCRCRCRCRCRCR Exceeded maximum number of defeated thieves\n");
				*terminateFlagp = 1;
				break;
			}
			usleep( (totalThiefInterval*800) );
		}




		while( (w = waitpid( -1, &status, WNOHANG)) > 1){
		       	if (WIFEXITED(status)){
			       	if (WEXITSTATUS(status) > 0) {
					printf("exited, status=%8d\n", WEXITSTATUS(status));
					terminateSimulation();
					printf("Exit %8d\n", getpid());
					exit(0);
				}
				else {
					printf("                           TERMINATED process %8d\n", w);
				}
			}
		}

		if( *terminateFlagp == 1 )break;

	}
	if( *terminateFlagp == 1 ) {
		terminateSimulation();
	}
	printf("Exit %8d\n", getpid());
	exit(0);
}


void smaug()
{
	int k;
	int temp;
	int localpid;
	double elapsedTime;

	/* local counters used only for smaug routine */
	int cowsEatenTotal = 0;
    int sheepEatenTotal = 0;
    int numJewels = INITIAL_TREASURE_IN_HOARD;
	int thievesDefeatedTotal = 0;
	int huntersDefeatedTotal = 0;
	int sleepThisIteration = 1;
	int terminateNow = 0;

	/* Initialize random number generator*/
	/* Random numbers are used to determine the time between successive beasts */
	smaugProcessID = getpid();
	printf("SMAUGSMAUGSMAUGSMAUGSMAU   PID is %d \n", smaugProcessID );
	localpid = smaugProcessID;
	printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has gone to sleep\n" );
	semopChecked(semID, &WaitDragonSleeping, 1);
	printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has woken up \n" );
	while (TRUE) {
		semopChecked(semID, &WaitProtectHuntersWaitingFlag, 1);
		if(*huntersWaitingFlagp >= 1){
			*huntersWaitingFlagp = *huntersWaitingFlagp - 1;
			semopChecked(semID, &SignalProtectHuntersWaitingFlag, 1);

			semopChecked(semID, &SignalHuntersWaiting, 1);

			printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug is waiting to fight a hunter\n");
			semopChecked(semID, &WaitDragonFighting, 1);
			huntersFoughtTotal++;

			semopChecked(semID, &WaitFightEnded, 1);

			semopChecked(semID, &WaitProtectFightResults, 1);
			if(*fightResultsp == 1){
				smaugsHoard = smaugsHoard - 10;
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug gave hunter 10 jewels for fighting well. %d jewels remain\n",smaugsHoard);
			}
			else{
				smaugsHoard = smaugsHoard + 5;
				*defeatedHuntersCountp = *defeatedHuntersCountp + 1;
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug received 5 jewels bribe from hunter.he now has %d jewels\n",smaugsHoard);
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   number of defeated hunters: %d\n",*defeatedHuntersCountp);
			}
			semopChecked(semID, &SignalProtectFightResults, 1);
			printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has fought hunter %d \n", huntersFoughtTotal);

			if(*defeatedHuntersCountp >= MAX_HUNTERS_DEFEATED) {
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has defeated the allowed number of hunters\n");
				*terminateFlagp= 1;
				break;
			}
			if(smaugsHoard <= 0 || smaugsHoard >= 800){
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has the allowed number of jewels\n");
				*terminateFlagp = 1;
				break;
			}
			printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug swims\n");
			usleep(1000);
			printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug finishes swim to smell another visitor\n");
		}
		else {
			semopChecked(semID, &SignalProtectHuntersWaitingFlag, 1);
		}

		semopChecked(semID, &WaitProtectThiefWaitingFlag, 1);
		if(*thiefWaitingFlagp >= 1){
			*thiefWaitingFlagp = *thiefWaitingFlagp - 1;
			semopChecked(semID, &SignalProtectThiefWaitingFlag, 1);

			semopChecked(semID, &SignalThiefWaiting, 1);
			printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug is waiting to play with thief\n");
			semopChecked(semID, &WaitDragonPlaying, 1);
			thievesPlayedTotal++;
			semopChecked(semID, &WaitPlayEnded, 1);

			semopChecked(semID, &WaitProtectFightResults, 1);
			if(*fightResultsp == 1){
				smaugsHoard = smaugsHoard - 8;
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug gave thief 8 jewels for winning game. %d jewels remain\n",smaugsHoard);
			}
			else{
				smaugsHoard = smaugsHoard + 20;
				*defeatedThievesCountp = *defeatedThievesCountp + 1;
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug received 20 jewels bribe from thief.he now has %d jewels\n",smaugsHoard);
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   number of defeated thieves: %d\n",*defeatedThievesCountp);
			}
			semopChecked(semID, &SignalProtectFightResults, 1);
			printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has played with thief %d \n", thievesPlayedTotal);

			if( *defeatedThievesCountp >= MAX_THIEVES_DEFEATED ) {
				printf("CRCRCRCRCRCRCRCRCRCRCRCR Exceeded maximum number of defeated thieves\n");
				*terminateFlagp = 1;
				break;
			}

			if(smaugsHoard <= 0 || smaugsHoard >= 800){
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has the allowed number of jewels\n");
				*terminateFlagp = 1;
				break;
			}
			printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug takes swim\n");
			usleep(1000);
			printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug finishes swim to smell another visitor\n");
		}
		else{
			semopChecked(semID, &SignalProtectThiefWaitingFlag, 1);
		}


		semopChecked(semID, &WaitProtectMealWaitingFlag, 1);
		while( *mealWaitingFlagp >= 1  ) {
			*mealWaitingFlagp = *mealWaitingFlagp - 1;
			printf("SMAUGSMAUGSMAUGSMAUGSMAU   signal meal flag %d\n", *mealWaitingFlagp);
			semopChecked(semID, &SignalProtectMealWaitingFlag, 1);
			printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug is eating a meal\n");
			for( k = 0; k < COWS_IN_GROUP; k++ ) {
				semopChecked(semID, &SignalCowsWaiting, 1);
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   A cow is ready to eat\n");
			}
            for( k = 0; k < SHEEP_IN_GROUP, k++){
                semopChcked(semID, &SignalSheepWaiting, 1);
                printf("SMAUGSMAUGSMAUGSMAUGSMAU   A sheep is ready to eat\n");
            }
			/*Smaug waits to eat*/
			semopChecked(semID, &WaitDragonEating, 1);
			for( k = 0; k < COWS_IN_GROUP; k++ ) {
				semopChecked(semID, &SignalCowsDead, 1);
				cowsEatenTotal++;
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug finished eating a cow\n");
			}
			for( k = 0; k < SHEEP_IN_GROUP; k++ ) {
				semopChecked(semID, &SignalSheepDead, 1);
				sheepEatenTotal++;
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug finished eating a sheep\n");
			}
			printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has finished a meal\n");
			if(cowsEatenTotal >= MAX_COWS_EATEN ) {
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has eaten the allowed number of cows\n");
				*terminateFlagp= 1;
				break;
			}

			/* Smaug checke to see if another snack is waiting */
			semopChecked(semID, &WaitProtectMealWaitingFlag, 1);
			if( *mealWaitingFlagp > 0  ) {
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug eats again\n", localpid);
				continue;
			}
			else {
				semopChecked(semID, &SignalProtectMealWaitingFlag, 1);
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug sleeps again\n", localpid);
				semopChecked(semID, &WaitDragonSleeping, 1);
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug is awake again\n", localpid);
				break;
			}
		}

	}
}


void initialize()
{
	/* Init semaphores */
	semID=semget(IPC_PRIVATE, 25, 0666 | IPC_CREAT);


	/* Init to zero, no elements are produced yet */
	seminfo.val=0;
	semctlChecked(semID, SEM_COWSINGROUP, SETVAL, seminfo);
	semctlChecked(semID, SEM_COWSWAITING, SETVAL, seminfo);
	semctlChecked(semID, SEM_COWSEATEN, SETVAL, seminfo);
	semctlChecked(semID, SEM_COWSDEAD, SETVAL, seminfo);

	semctlChecked(semID, SEM_SHEEPSINGROUP, SETVAL, seminfo);
	semctlChecked(semID, SEM_SHEEPWAITING, SETVAL, seminfo);
	semctlChecked(semID, SEM_SHEEPEATEN, SETVAL, seminfo);
	semctlChecked(semID, SEM_SHEEPDEAD, SETVAL, seminfo);

	semctlChecked(semID, SEM_DRAGONFIGHTING, SETVAL, seminfo);
	semctlChecked(semID, SEM_DRAGONSLEEPING, SETVAL, seminfo);
	semctlChecked(semID, SEM_DRAGONEATING, SETVAL, seminfo);
	semctlChecked(semID, SEM_HUNTERSWAITING, SETVAL, seminfo);
	semctlChecked(semID, SEM_HUNTERSFOUGHT, SETVAL, seminfo);
	semctlChecked(semID, SEM_THIEVESWAITING, SETVAL, seminfo);
	semctlChecked(semID, SEM_THEIVESFOUGHT, SETVAL, seminfo);
	printf("!!INIT!!INIT!!INIT!!  semaphores initiialized\n");

	/* Init Mutex to one */
	seminfo.val=1;
	semctlChecked(semID, SEM_PCOWSINGROUP, SETVAL, seminfo);
	semctlChecked(semID, SEM_PSHEEPINGROUP, SETVAL, seminfo);
	semctlChecked(semID, SEM_PMEALWAITINGFLAG, SETVAL, seminfo);
	semctlChecked(semID, SEM_PCOWSEATEN, SETVAL, seminfo);
	semctlChechedk(semID, SEM_PSHEEPEATEN, SETVAL, seminfo);
	semctlChecked(semID, SEM_PTERMINATE, SETVAL, seminfo);
	semctlChecked(semID, SEM_PTHIEFCOUNT, SETVAL, seminfo);
	semctlChecked(semID, SEM_PHUNTERCOUNT, SETVAL, seminfo);
	printf("!!INIT!!INIT!!INIT!!  mutexes initiialized\n");


	/* Now we create and attach  the segments of shared memory*/
        if ((terminateFlag = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
                printf("!!INIT!!INIT!!INIT!!  shm not created for terminateFlag\n");
                 exit(1);
        }
        else {
                printf("!!INIT!!INIT!!INIT!!  shm created for terminateFlag\n");
        }
	if ((cowCounter = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
		printf("!!INIT!!INIT!!INIT!!  shm not created for cowCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm created for cowCounter\n");
	}
	if ((sheepCounter = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
		printf("!!INIT!!INIT!!INIT!!  shm not created for sheepCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm created for sheepCounter\n");
	}
	if ((mealWaitingFlag = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
		printf("!!INIT!!INIT!!INIT!!  shm not created for mealWaitingFlag\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm created for mealWaitingFlag\n");
	}
	if ((cowsEatenCounter = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
		printf("!!INIT!!INIT!!INIT!!  shm not created for cowsEatenCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm created for cowsEatenCounter\n");
	}
	if ((sheepEatenCounter = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
		printf("!!INIT!!INIT!!INIT!!  shm not created for sheepEatenCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm created for sheepEatenCounter\n");
	}


	/* Now we attach the segment to our data space.  */
        if ((terminateFlagp = shmat(terminateFlag, NULL, 0)) == (int *) -1) {
                printf("!!INIT!!INIT!!INIT!!  shm not attached for terminateFlag\n");
                exit(1);
        }
        else {
                 printf("!!INIT!!INIT!!INIT!!  shm attached for terminateFlag\n");
        }

	if ((cowCounterp = shmat(cowCounter, NULL, 0)) == (int *) -1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for cowCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for cowCounter\n");
	}
	if ((csheepCounterp = shmat(sheepCounter, NULL, 0)) == (int *) -1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for sheepCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for sheepCounter\n");
	}
	if ((mealWaitingFlagp = shmat(mealWaitingFlag, NULL, 0)) == (int *) -1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for mealWaitingFlag\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for mealWaitingFlag\n");
	}
	if ((cowsEatenCounterp = shmat(cowsEatenCounter, NULL, 0)) == (int *) -1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for cowsEatenCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for cowsEatenCounter\n");
	}
	if ((sheepEatenCounterp = shmat(sheepEatenCounter, NULL, 0)) == (int *) -1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for sheepEatenCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for sheepEatenCounter\n");
	}
	if ((Prob_Winp = shmat(Prob_Winp, NULL, 0)) == (int *) -1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for Prob_Winp\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for Prob_Winp\n");
	}
	if ((defeatedHuntersCountp = shmat(defeatedHuntersCount, NULL, 0)) == (int *) -1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for defeatedHuntersCount\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for defeatedHuntersCount\n");
	}
	if ((thiefWaitingFlagp = shmat(thiefWaitingFlag, NULL, 0)) == (int *) -1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for thiefWaitingFlag\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for thiefWaitingFlag\n");
	}
	if ((defeatedThievesCountp = shmat(defeatedThievesCount, NULL, 0)) == (int *) -1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for defeatedThievesCount\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for defeatedThievesCount\n");
	}
	if ((huntersWaitingFlagp = shmat(huntersWaitingFlag, NULL, 0)) == (int *) -1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for huntersWaitingFlag\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for huntersWaitingFlag\n");
	}
	printf("!!INIT!!INIT!!INIT!!   initialize end\n");
}



void cow(int startTimeN)
{
	int localpid;
	int retval;
	int k;
	localpid = getpid();

	/* graze */
	printf("CCCCCCC %8d CCCCCCC   A cow is born\n", localpid);
	if( startTimeN > 0) {
		if( usleep( startTimeN) == -1){
			/* exit when usleep interrupted by kill signal */
			if(errno==EINTR)
			    exit(4);
		}
	}
	printf("CCCCCCC %8d CCCCCCC   cow grazes for %f ms\n", localpid, startTimeN/1000.0);


	/* does this beast complete a group of BEASTS_IN_GROUP ? */
	/* if so wake up the dragon */
	semopChecked(semID, &WaitProtectCowsInGroup, 1);
	semopChecked(semID, &SignalCowsInGroup, 1);
	*cowCounterp = *cowCounterp + 1;
	printf("CCCCCCC %8d CCCCCCC   %d  cows have been enchanted \n", localpid, *cowCounterp );
	if( ( *cowCounterp  >= COWS_IN_GROUP )) {
		*cowCounterp = *cowCounterp - COWS_IN_GROUP;
		for (k=0; k<COWS_IN_GROUP; k++){
			semopChecked(semID, &WaitCowsInGroup, 1);
		}
		printf("CCCCCCC %8d CCCCCCC   The last cow is waiting\n", localpid);
		semopChecked(semID, &SignalProtectCowsInGroup, 1);
		semopChecked(semID, &WaitProtectMealWaitingFlag, 1);
		*mealWaitingFlagp = *mealWaitingFlagp + 1;
		printf("CCCCCCC %8d CCCCCCC   signal meal flag %d\n", localpid, *mealWaitingFlagp);
		semopChecked(semID, &SignalProtectMealWaitingFlag, 1);
		semopChecked(semID, &SignalDragonSleeping, 1);
		printf("CCCCCCC %8d CCCCCCC   last cow  wakes the dragon \n", localpid);
	}
	else
	{
		semopChecked(semID, &SignalProtectCowsInGroup, 1);
	}

	semopChecked(semID, &WaitCowsWaiting, 1);

	/* have all the beasts in group been eaten? */
	/* if so wake up the dragon */
	semopChecked(semID, &WaitProtectCowsEaten, 1);
	semopChecked(semID, &SignalCowsEaten, 1);
	*cowsEatenCounterp = *cowsEatenCounterp + 1;
	if( ( *cowsEatenCounterp >= COWS_IN_GROUP )) {
		*cowsEatenCounterp = *cowsEatenCounterp - COWS_IN_GROUP;
		for (k=0; k<COWS_IN_GROUP; k++){
       		        semopChecked(semID, &WaitCowsEaten, 1);
		}
		printf("CCCCCCC %8d CCCCCCC   The last cow has been eaten\n", localpid);
		semopChecked(semID, &SignalProtectCowsEaten, 1);
		semopChecked(semID, &SignalDragonEating, 1);
	}
	else
	{
		semopChecked(semID, &SignalProtectCowsEaten, 1);
		printf("CCCCCCC %8d CCCCCCC   A cow is waiting to be eaten\n", localpid);
	}
	semopChecked(semID, &WaitCowsDead, 1);

	printf("CCCCCCC %8d CCCCCCC   cow  dies\n", localpid);
}

void sheep(int startTimeN)
{
	int localpid;
	int retval;
	int k;
	localpid = getpid();

	/* graze */
	printf("SSSSSSS %8d SSSSSSS   A sheep is born\n", localpid);
	if( startTimeN > 0) {
		if( usleep( startTimeN) == -1){
			/* exit when usleep interrupted by kill signal */
			if(errno==EINTR)
			    exit(4);
		}
	}
	printf("SSSSSSS %8d SSSSSSS   sheep grazes for %f ms\n", localpid, startTimeN/1000.0);


	/* does this beast complete a group of BEASTS_IN_GROUP ? */
	/* if so wake up the dragon */
	semopChecked(semID, &WaitProtectSheepInGroup, 1);
	semopChecked(semID, &SignalSheepInGroup, 1);
	*sheepCounterp = *sheepCounterp + 1;
	printf("SSSSSSS %8d SSSSSSS   %d  sheep have been enchanted \n", localpid, *sheepCounterp );
	if( ( *sheepCounterp  >= SHEEP_IN_GROUP )) {
		*sheepCounterp = *sheepCounterp - SHEEP_IN_GROUP;
		for (k=0; k<COWS_IN_GROUP; k++){
			semopChecked(semID, &WaitCowsInGroup, 1);
		}
		printf("SSSSSSS %8d SSSSSSS   The last sheep is waiting\n", localpid);
		semopChecked(semID, &SignalProtectSheepInGroup, 1);
		semopChecked(semID, &WaitProtectMealWaitingFlag, 1);
		*mealWaitingFlagp = *mealWaitingFlagp + 1;
		printf("SSSSSSS %8d SSSSSSS   signal meal flag %d\n", localpid, *mealWaitingFlagp);
		semopChecked(semID, &SignalProtectMealWaitingFlag, 1);
		semopChecked(semID, &SignalDragonSleeping, 1);
		printf("SSSSSSS %8d SSSSSSS   last sheep  wakes the dragon \n", localpid);
	}
	else
	{
		semopChecked(semID, &SignalProtectSheepInGroup, 1);
	}

	semopChecked(semID, &WaitSheepWaiting, 1);

	/* have all the beasts in group been eaten? */
	/* if so wake up the dragon */
	semopChecked(semID, &WaitProtectSheepEaten, 1);
	semopChecked(semID, &SignalSheepEaten, 1);
	*sheepEatenCounterp = *sheepEatenCounterp + 1;
	if( ( *sheepEatenCounterp >= SHEEP_IN_GROUP )) {
		*sheepEatenCounterp = *sheepEatenCounterp - SHEEP_IN_GROUP;
		for (k=0; k<SHEEP_IN_GROUP; k++){
       		        semopChecked(semID, &WaitSheepEaten, 1);
		}
		printf("SSSSSSS %8d SSSSSSS   The last sheep has been eaten\n", localpid);
		semopChecked(semID, &SignalProtectSheepEaten, 1);
		semopChecked(semID, &SignalDragonEating, 1);
	}
	else
	{
		semopChecked(semID, &SignalProtectSheepEaten, 1);
		printf("SSSSSSS %8d SSSSSSS   A sheep is waiting to be eaten\n", localpid);
	}
	semopChecked(semID, &WaitSheepDead, 1);

	printf("SSSSSSS %8d SSSSSSS   sheep  dies\n", localpid);
}


void hunter(int startTimeN){
	int localpid;
	localpid = getpid();

	printf("HHHHHH %8d HHHHHHH   A hunter is born\n", localpid);
	if(startTimeN > 0){
		if(usleep( startTimeN) == -1){
			if(errno==EINTR)exit(4);
		}
	}
	printf("HHHHHH %8d HHHHHH   hunter travels for %f ms\n", localpid, startTimeN/1000.0);

	semopChecked(semID, &WaitProtectHuntersWaitingFlag, 1);
	*huntersWaitingFlagp = *huntersWaitingFlagp + 1;
	semopChecked(semID, &SignalProtectHuntersWaitingFlag, 1);

	semopChecked(semID, &SignalDragonSleeping,1);

	printf("HHHHHH %8d HHHHHHH   A hunter is waiting\n", localpid);
	semopChecked(semID, &WaitHuntersWaiting, 1);

	semopChecked(semID, &SignalDragonFighting, 1);

	printf("HHHHHH %8d HHHHHHH   A hunter is fighting smaug!\n", localpid);
	semopChecked(semID, &WaitProtectFightResults, 1);
	*fightResultsp = randlim(*Prob_Winp);
	if(*fightResultsp == 1){
		printf("HHHHHH %8d HHHHHHH   hunter fought well and was given 10 jewels \n", localpid);
	}
	else{
		printf("HHHHHH %8d HHHHHHH   hunter lost the fight and bribes smaug with 5 jewels \n", localpid);
	}
	semopChecked(semID, &SignalProtectFightResults, 1);

	semopChecked(semID, &SignalFightEnded, 1);

	printf("HHHHHH %8d HHHHHHH   hunter has fled the cave\n", localpid);
}

void thief(int startTimeN){
	int localpid;
	localpid = getpid();

	printf("TTTTTTT %8d TTTTTTT   A thief is born\n", localpid);
	if(startTimeN > 0){
		if(usleep( startTimeN) == -1){
			if(errno==EINTR)exit(4);
		}
	}
	printf("TTTTTTT %8d TTTTTTT   thief travels for %f ms\n", localpid, startTimeN/1000.0);

	semopChecked(semID, &WaitProtectThiefWaitingFlag, 1);
	*thiefWaitingFlagp = *thiefWaitingFlagp + 1;
	semopChecked(semID, &SignalProtectThiefWaitingFlag, 1);

	semopChecked(semID, &SignalDragonSleeping, 1);

	printf("TTTTTTT %8d TTTTTTT   A thief is waiting\n", localpid);
	semopChecked(semID, &WaitThiefWaiting, 1);
	semopChecked(semID, &SignalDragonPlaying, 1);
	printf("TTTTTTT %8d TTTTTTT   A thief is playing with smaug\n", localpid);

	semopChecked(semID, &WaitProtectFightResults, 1);
	*fightResultsp = randlim(*Prob_Winp);
	if(*fightResultsp == 1){
		printf("TTTTTTT %8d TTTTTTT   thief won the game and was given 8 jewels \n", localpid);
	}
	else{
		printf("TTTTTTT %8d TTTTTTT   thief lost the game and bribes smaug with 20 jewels \n", localpid);
	}
	semopChecked(semID, &SignalProtectFightResults, 1);
	semopChecked(semID, &SignalPlayEnded, 1);
	printf("TTTTTTT %8d TTTTTTT   thief has fled the cave\n", localpid);

}


void terminateSimulation() {
	pid_t localpgid;
	pid_t localpid;
	int w = 0;
	int status;

	localpid = getpid();
	printf("RELEASESEMAPHORES   Terminating Simulation from process %8d\n", localpgid);
	if(cowProcessGID != (int)localpgid ){
		if(killpg(cowProcessGID, SIGKILL) == -1 && errno == EPERM) {
			printf("XXTERMINATETERMINATE   COWS NOT KILLED\n");
		}
		printf("XXTERMINATETERMINATE   killed cows \n");
	}
    if(sheepProcessGID != (int)localpgid ){
		if(killpg(sheepProcessGID, SIGKILL) == -1 && errno == EPERM) {
			printf("XXTERMINATETERMINATE   SHEEPS NOT KILLED\n");
		}
		printf("XXTERMINATETERMINATE   killed sheep \n");
	}
	if(hunterProcessGID != (int)localpgid ){
		if(killpg(hunterProcessGID, SIGKILL) == -1 && errno == EPERM) {
			printf("XXTERMINATETERMINATE   HUNTERS NOT KILLED\n");
		}
		printf("XXTERMINATETERMINATE   killed hunters \n");
	}
	if(thiefProcessGID != (int)localpgid ){
		if(killpg(thiefProcessGID, SIGKILL) == -1 && errno == EPERM) {
			printf("XXTERMINATETERMINATE   THIEVES NOT KILLED\n");
		}
		printf("XXTERMINATETERMINATE   killed thieves \n");
	}
	if(smaugProcessID != (int)localpgid ) {
		kill(smaugProcessID, SIGKILL);
		printf("XXTERMINATETERMINATE   killed smaug\n");
	}
	while( (w = waitpid( -1, &status, WNOHANG)) > 1){
			printf("                           REAPED process in terminate %d\n", w);
	}
	releaseSemandMem();
	printf("GOODBYE from terminate\n");
}

void releaseSemandMem()
{
	pid_t localpid;
	int w = 0;
	int status;

	localpid = getpid();

	//should check return values for clean termination
	semctl(semID, 0, IPC_RMID, seminfo);


	// wait for the semaphores
	usleep(2000);
	while( (w = waitpid( -1, &status, WNOHANG)) > 1){
			printf("                           REAPED process in terminate %d\n", w);
	}
	printf("\n");
        if(shmdt(terminateFlagp)==-1) {
                printf("RELEASERELEASERELEAS   terminateFlag share memory detach failed\n");
        }
        else{
                printf("RELEASERELEASERELEAS   terminateFlag share memory detached\n");
        }
        if( shmctl(terminateFlag, IPC_RMID, NULL ))
        {
                printf("RELEASERELEASERELEAS   share memory delete failed %d\n",*terminateFlagp );
        }
        else{
                printf("RELEASERELEASERELEAS   share memory deleted\n");
        }
	if( shmdt(cowCounterp)==-1)
	{
		printf("RELEASERELEASERELEAS   cowCounterp memory detach failed\n");
	}
	else{
		printf("RELEASERELEASERELEAS   cowCounterp memory detached\n");
	}
	if( shmctl(cowCounter, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   cowCounter memory delete failed \n");
	}
	else{
		printf("RELEASERELEASERELEAS   cowCounter memory deleted\n");
	}
	if( shmdt(sheepCounterp)==-1)
	{
		printf("RELEASERELEASERELEAS   sheepCounterp memory detach failed\n");
	}
	else{
		printf("RELEASERELEASERELEAS   sheepCounterp memory detached\n");
	}
	if( shmctl(sheepCounter, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   sheepCounter memory delete failed \n");
	}
	else{
		printf("RELEASERELEASERELEAS   sheepCounter memory deleted\n");
	}
	if( shmdt(mealWaitingFlagp)==-1)
	{
		printf("RELEASERELEASERELEAS   mealWaitingFlagp memory detach failed\n");
	}
	else{
		printf("RELEASERELEASERELEAS   mealWaitingFlagp memory detached\n");
	}
	if( shmctl(mealWaitingFlag, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   mealWaitingFlag share memory delete failed \n");
	}
	else{
		printf("RELEASERELEASERELEAS   mealWaitingFlag share memory deleted\n");
	}
	if( shmdt(cowsEatenCounterp)==-1)
	{
		printf("RELEASERELEASERELEAS   cowsEatenCounterp memory detach failed\n");
	}
	else{
		printf("RELEASERELEASERELEAS   cowsEatenCounterp memory detached\n");
	}
	if( shmctl(cowsEatenCounter, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   cowsEatenCounter memory delete failed \n");
	}
	else{
		printf("RELEASERELEASERELEAS   cowsEatenCounter memory deleted\n");
	}
    if( shmdt(hunterCounterp)==-1)
	{
		printf("RELEASERELEASERELEAS   hunterCounterp memory detach failed\n");
	}
	else{
		printf("RELEASERELEASERELEAS   hunterCounterp memory detached\n");
	}
	if( shmctl(hunterCounter, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   hunterCounter memory delete failed \n");
	}
	else{
		printf("RELEASERELEASERELEAS   hunterCounter memory deleted\n");
	}
	// THIEF MEMORY
	if( shmdt(thiefCounterp)==-1)
	{
		printf("RELEASERELEASERELEAS   thiefCounterp memory detach failed\n");
	}
	else{
		printf("RELEASERELEASERELEAS   thiefCounterp memory detached\n");
	}
	if( shmctl(thiefCounter, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   thiefCounter memory delete failed \n");
	}
	else{
		printf("RELEASERELEASERELEAS   thiefCounter memory deleted\n");
	}
}
}

void semctlChecked(int semaphoreID, int semNum, int flag, union semun seminfo) {
	/* wrapper that checks if the semaphore control request has terminated */
	/* successfully. If it has not the entire simulation is terminated */

	if (semctl(semaphoreID, semNum, flag,  seminfo) == -1 ) {
		if(errno != EIDRM) {
			printf("semaphore control failed: simulation terminating\n");
			printf("errno %8d \n",errno );
			*terminateFlagp = 1;
			releaseSemandMem();
			exit(2);
		}
		else {
			exit(3);
		}
	}
}

void semopChecked(int semaphoreID, struct sembuf *operation, unsigned something)
{

	/* wrapper that checks if the semaphore operation request has terminated */
	/* successfully. If it has not the entire simulation is terminated */
	if (semop(semaphoreID, operation, something) == -1 ) {
		if(errno != EIDRM) {
			printf("semaphore operation failed: simulation terminating\n");
			*terminateFlagp = 1;
			releaseSemandMem();
			exit(2);
		}
		else {
			exit(3);
		}
	}
}

int randlim(int limit) {

    int divisor = RAND_MAX/(limit+1);
    int retval;

    do {
        retval = rand() / divisor;
    } while (retval > limit);

    return retval;
}

double timeChange( const struct timeval startTime )
{
	struct timeval nowTime;
	double elapsedTime;

	gettimeofday(&nowTime,NULL);
	elapsedTime = (nowTime.tv_sec - startTime.tv_sec)*1000.0;
	elapsedTime +=  (nowTime.tv_usec - startTime.tv_usec)/1000.0;
	return elapsedTime;

}
