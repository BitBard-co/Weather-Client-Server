#ifndef StateMachineWorker_h__
#define StateMachineWorker_h__

#include <sys/types.h>

#ifdef __linux__
	#include <unistd.h>
#endif


#include "LinkedList.h"


struct T_StateMachineWorker;
typedef struct T_StateMachineWorker StateMachineWorker;

struct T_StateMachineWorker_Task;
typedef struct T_StateMachineWorker_Task StateMachineWorker_Task;

#include "wString.h"

//#define STATEMACHINE_ENABLE_STATS
#define STATEMACHINE_ENABLE_STATS_SAMPLE_RATE 1
//#define STATEMACHINE_ENABLE_THROTTLE

#ifdef STATEMACHINE_ENABLE_STATS
	typedef struct
	{
		UInt32 m_CountMain;
		UInt32 m_Count;
		UInt32 m_Time;

	} StateMachineWorker_Stats_Data;

	typedef struct
	{
		Bool m_Enabled;
		Bool m_Visual;
		StateMachineWorker_Stats_Data m_Data;
		UInt32 m_IterationsPerSec;
		UInt32 m_MainWhile;

		UInt32 m_MainWhileCount;
		UInt32 m_MainWhileTotal;
		UInt32 m_MainWhileAvg;

		clock_t m_TotalClock;
		UInt32 m_Timeout;

		wString m_ReportStr;

	} StateMachineWorker_Stats;
#endif

#ifdef STATEMACHINE_ENABLE_THROTTLE
	typedef struct
	{
		Bool m_Enabled;
		Bool m_Slow;

		int m_Target;
		UInt64 m_SampleTime;
		int m_Hysteresis;
		unsigned int m_Speed;

		UInt64 m_Next;
		unsigned int m_Counter;

		#ifdef __APPLE__
			useconds_t us;
		#else
			unsigned int us;
		#endif

} StateMachineWorker_Throttle;
#endif

struct T_StateMachineWorker
{
	LinkedList m_List;
	LinkedList m_PrioList;
	LinkedList_Node* m_CurrentNode;
	#ifdef STATEMACHINE_ENABLE_STATS
		StateMachineWorker_Stats m_Stats;
	#endif
	Bool m_Disposed;
	UInt32 m_tIDUbound;

	#ifdef STATEMACHINE_ENABLE_THROTTLE
		StateMachineWorker_Throttle m_Throttle;
	#endif
};


struct T_StateMachineWorker_Task
{
	void* m_Object;
	void* m_Context;
	Bool m_Delete;
	Bool m_Removed;
	void (*m_FuncPtr)(StateMachineWorker_Task* _Task, UInt64 _MonTime);
	UInt64 m_Suspended;
	UInt64 m_SuspendUntil;

	#ifdef STATEMACHINE_ENABLE_STATS
		clock_t m_Clock;
		wString m_Name;
		UInt32 m_tID;
		float m_CPU;


		float m_MAX;
		float m_CurrMAX;

		float m_AVG;
		float m_AverageValue;
		unsigned int m_AverageCount;

	#endif
};

extern StateMachineWorker g_StateMachineWorker;

void StateMachineWorker_Initialize();

#ifdef STATEMACHINE_ENABLE_THROTTLE
	void StateMachineWorker_SetThrottle(Bool _Enabled, int _Target, int _Hysteresis, unsigned int _Speed, UInt64 _SampleTime);
#endif

#ifdef STATEMACHINE_ENABLE_STATS
	void StateMachineWorker_ToggleStats();
	void StateMachineWorker_EnableStats();
	void StateMachineWorker_DisableStats();
#endif

UInt64 StateMachineWorker_Work(UInt64* _MonTime);
StateMachineWorker_Task* StateMachineWorker_CreateTask(void* _Object, void (*_FuncPtr)(StateMachineWorker_Task* _Task, UInt64 _MonTime), const char* _Name, const char* _SubName);
StateMachineWorker_Task* StateMachineWorker_CreatePrio(void* _Object, void (*_FuncPtr)(StateMachineWorker_Task* _Task, UInt64 _MonTime));

void StateMachineWorker_SuspendTask(StateMachineWorker_Task* _Task, UInt64 _AutoResume);
void StateMachineWorker_SuspendTaskUntil(StateMachineWorker_Task* _Task, UInt64 _UTC);
void StateMachineWorker_ResumeTask(StateMachineWorker_Task* _Task);

void StateMachineWorker_DeleteTask(StateMachineWorker_Task* _Task);
void StateMachineWorker_Abort(StateMachineWorker_Task* _Task);
void StateMachineWorker_Dispose();

#endif // StateMachineWorker_h__