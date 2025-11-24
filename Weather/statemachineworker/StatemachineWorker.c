#include "StateMachineWorker.h"
#include "WAllocator.h"
#include <math.h>
#include <stdlib.h>

StateMachineWorker g_StateMachineWorker;


void StateMachineWorker_Initialize()
{
	LinkedList_Initialize(&g_StateMachineWorker.m_List);
	LinkedList_Initialize(&g_StateMachineWorker.m_PrioList);
	g_StateMachineWorker.m_CurrentNode = NULL;
	g_StateMachineWorker.m_Disposed = False;
	g_StateMachineWorker.m_tIDUbound = 0;

	#ifdef STATEMACHINE_ENABLE_THROTTLE
		memset(&g_StateMachineWorker.m_Throttle, 0, sizeof(g_StateMachineWorker.m_Throttle));
	#endif


	#ifdef STATEMACHINE_ENABLE_STATS
		memset(&g_StateMachineWorker.m_Stats, 0, sizeof(g_StateMachineWorker.m_Stats));
		g_StateMachineWorker.m_Stats.m_Timeout = SystemMonotonic() + STATEMACHINE_ENABLE_STATS_SAMPLE_RATE;
		g_StateMachineWorker.m_Stats.m_Enabled = False;
		g_StateMachineWorker.m_Stats.m_Visual = False;

		#ifdef STATEMACHINE_ENABLE_STATS
			wString_Initialize(&g_StateMachineWorker.m_Stats.m_ReportStr, 1024);
		#endif
	#endif
}

#ifdef STATEMACHINE_ENABLE_THROTTLE
	void StateMachineWorker_SetThrottle(Bool _Enabled, int _Target, int _Hysteresis, unsigned int _Speed, UInt64 _SampleTime)
	{
		if(_Enabled == False)
		{
			memset(&g_StateMachineWorker.m_Throttle, 0, sizeof(g_StateMachineWorker.m_Throttle));
			return;
		}

		g_StateMachineWorker.m_Throttle.m_Enabled = _Enabled;
		g_StateMachineWorker.m_Throttle.m_Slow = False;
		g_StateMachineWorker.m_Throttle.m_Target = _Target;
		g_StateMachineWorker.m_Throttle.m_Hysteresis = _Hysteresis;
		g_StateMachineWorker.m_Throttle.m_Speed = _Speed;
		g_StateMachineWorker.m_Throttle.m_SampleTime = _SampleTime;

		SystemMonotonicMS(&g_StateMachineWorker.m_Throttle.m_Next);
		g_StateMachineWorker.m_Throttle.m_Next += g_StateMachineWorker.m_Throttle.m_SampleTime;

		g_StateMachineWorker.m_Throttle.m_Counter = 0;
		g_StateMachineWorker.m_Throttle.us = 100;
	}
#endif

#ifdef STATEMACHINE_ENABLE_STATS

	void StateMachineWorker_ToggleStats()
	{
		if(g_StateMachineWorker.m_Stats.m_Enabled == True)
			StateMachineWorker_DisableStats();
		else
			StateMachineWorker_EnableStats();

	}

	void StateMachineWorker_EnableStats()
	{
		system("clear");
		printf("StateMachineWorker stats enabled!\r\n");
		g_StateMachineWorker.m_Stats.m_Visual = True;
		g_StateMachineWorker.m_Stats.m_Enabled = True;
	}

	void StateMachineWorker_DisableStats()
	{
		g_StateMachineWorker.m_Stats.m_Visual = False;
		g_StateMachineWorker.m_Stats.m_Enabled = False;
		system("clear");
		printf("StateMachineWorker stats disabled!\r\n");
	}

#endif

UInt64 StateMachineWorker_Work(UInt64* _MonTime)
{
	if(g_StateMachineWorker.m_Disposed)
		return 0;

	UInt64 monTime;

	if(_MonTime == NULL)
		SystemMonotonicMS(&monTime);
	else
		monTime = *(_MonTime);

	UInt64 sysTime;
	SystemTimeMS(&sysTime);

	LinkedList_Node* node = g_StateMachineWorker.m_PrioList.m_HeadNode;
	while(node != NULL)
	{
		StateMachineWorker_Task* _Task = (StateMachineWorker_Task*)node->m_Item;
		node = node->m_NextNode;

		if(_Task->m_Delete == True)
		{
			LinkedList_Remove(&g_StateMachineWorker.m_PrioList, _Task);
			#ifdef STATEMACHINE_ENABLE_STATS
				wString_Dispose(&_Task->m_Name);
			#endif
			WAllocator_Free((unsigned char*)_Task);
		}
		else
		{
			_Task->m_FuncPtr(_Task, monTime);
		}
	}

	if(g_StateMachineWorker.m_CurrentNode == NULL)
		g_StateMachineWorker.m_CurrentNode = g_StateMachineWorker.m_List.m_HeadNode;


	if(g_StateMachineWorker.m_CurrentNode != NULL)
	{
		StateMachineWorker_Task* _Task = (StateMachineWorker_Task*)g_StateMachineWorker.m_CurrentNode->m_Item;
		g_StateMachineWorker.m_CurrentNode = g_StateMachineWorker.m_CurrentNode->m_NextNode;

		if(_Task->m_Delete == True)
		{
			LinkedList_Remove(&g_StateMachineWorker.m_List, _Task);
			#ifdef STATEMACHINE_ENABLE_STATS
				wString_Dispose(&_Task->m_Name);
			#endif
			WAllocator_Free((unsigned char*)_Task);
		}
		else
		{
			if(_Task->m_Suspended > 0)
			{
				if(monTime >= _Task->m_Suspended)
				{
					_Task->m_Suspended = 0;
					_Task->m_SuspendUntil = 0;
				}
				else
				{
					if(_Task->m_SuspendUntil > 0)
					{
						if(sysTime > _Task->m_SuspendUntil)
						{
							_Task->m_Suspended = 0;
							_Task->m_SuspendUntil = 0;
						}
					}
				}
			}

			if(_Task->m_Suspended == 0)
			{
				#ifdef STATEMACHINE_ENABLE_STATS
					clock_t start = clock();
				#endif

				_Task->m_FuncPtr(_Task, monTime);

				#ifdef STATEMACHINE_ENABLE_STATS
					if(g_StateMachineWorker.m_Stats.m_Enabled == True)
					{
						clock_t current = clock();
						clock_t total;
						if(current < start)
							total = current + start;
						else
							total = current - start;

						_Task->m_Clock += total;
						g_StateMachineWorker.m_Stats.m_TotalClock += total;
					}
				#endif
			}
		}

		#ifdef STATEMACHINE_ENABLE_THROTTLE
			if(g_StateMachineWorker.m_Throttle.m_Enabled == True)
			{
				if(g_StateMachineWorker.m_CurrentNode == NULL)
				{
					g_StateMachineWorker.m_Throttle.m_Counter++;

					if(monTime >= g_StateMachineWorker.m_Throttle.m_Next)
					{
						g_StateMachineWorker.m_Throttle.m_Next = monTime + g_StateMachineWorker.m_Throttle.m_SampleTime;
						g_StateMachineWorker.m_Throttle.m_Slow = False;

						float actual = (float)g_StateMachineWorker.m_Throttle.m_Counter / (float)(g_StateMachineWorker.m_Throttle.m_SampleTime / 1000.0);
						g_StateMachineWorker.m_Throttle.m_Counter = 0;

						if(fabs(actual - (float)g_StateMachineWorker.m_Throttle.m_Target) > g_StateMachineWorker.m_Throttle.m_Hysteresis)
						{
							if(actual < g_StateMachineWorker.m_Throttle.m_Target)
							{
								if(g_StateMachineWorker.m_Throttle.us > 10)
								{
									g_StateMachineWorker.m_Throttle.us -= g_StateMachineWorker.m_Throttle.m_Speed;
									//printf("StateMachineWorker.throttle changed to %2.4lfms (actual = %2.2f, target = %i)\r\n", ((double)g_StateMachineWorker.m_Throttle.us) / 1000.0, actual, g_StateMachineWorker.m_Throttle.m_Target);
								}
								else
								{
									//printf("Warning! Hit throttle bottom! (actual = %2.2f, target = %i)\r\n", actual, g_StateMachineWorker.m_Throttle.m_Target);
									g_StateMachineWorker.m_Throttle.m_Slow = True;
								}
							}
							else
							{
								g_StateMachineWorker.m_Throttle.us += g_StateMachineWorker.m_Throttle.m_Speed;
								//printf("StateMachineWorker.throttle changed to %2.4lfms (actual = %2.2f, target = %i)\r\n", ((double)g_StateMachineWorker.m_Throttle.us) / 1000.0, actual, g_StateMachineWorker.m_Throttle.m_Target);
							}
						}
					}
				}

				usleep(g_StateMachineWorker.m_Throttle.us);
			}
			else
			{
				usleep(0);
			}
		#endif


		#ifdef STATEMACHINE_ENABLE_STATS
			if(g_StateMachineWorker.m_Stats.m_Enabled == True)
				g_StateMachineWorker.m_Stats.m_Data.m_CountMain++;

			if(g_StateMachineWorker.m_CurrentNode == NULL)
			{

				if(g_StateMachineWorker.m_Stats.m_Enabled == True)
				{
					g_StateMachineWorker.m_Stats.m_Data.m_Count++;

					if(SystemMonotonic() >= g_StateMachineWorker.m_Stats.m_Timeout)
					{
						g_StateMachineWorker.m_Stats.m_IterationsPerSec = (UInt32)((float)g_StateMachineWorker.m_Stats.m_Data.m_Count / (float)STATEMACHINE_ENABLE_STATS_SAMPLE_RATE);

						g_StateMachineWorker.m_Stats.m_MainWhile = (UInt32)((float)g_StateMachineWorker.m_Stats.m_Data.m_CountMain / (float)STATEMACHINE_ENABLE_STATS_SAMPLE_RATE);
						g_StateMachineWorker.m_Stats.m_MainWhileTotal += g_StateMachineWorker.m_Stats.m_MainWhile;
						g_StateMachineWorker.m_Stats.m_MainWhileCount++;
						if(g_StateMachineWorker.m_Stats.m_MainWhileCount > 10)
						{
							if(g_StateMachineWorker.m_Stats.m_MainWhileCount == 0 || g_StateMachineWorker.m_Stats.m_MainWhileTotal == 0)
								g_StateMachineWorker.m_Stats.m_MainWhileAvg = 0;
							else
								g_StateMachineWorker.m_Stats.m_MainWhileAvg = g_StateMachineWorker.m_Stats.m_MainWhileTotal / g_StateMachineWorker.m_Stats.m_MainWhileCount;

							g_StateMachineWorker.m_Stats.m_MainWhileCount = 0;
							g_StateMachineWorker.m_Stats.m_MainWhileTotal = 0;

							if(g_StateMachineWorker.m_Stats.m_MainWhileAvg == 0) //Enables the possibility to check if the first ten seconds has passed before commiting an alarm. If m_MainWhileAvg == 0 then no data available
								g_StateMachineWorker.m_Stats.m_MainWhileAvg = 1;

						}

						g_StateMachineWorker.m_Stats.m_Data.m_Time = SystemMonotonic() + STATEMACHINE_ENABLE_STATS_SAMPLE_RATE;
						g_StateMachineWorker.m_Stats.m_Data.m_Count = 0;
						g_StateMachineWorker.m_Stats.m_Data.m_CountMain = 0;

						wString_Clear(&g_StateMachineWorker.m_Stats.m_ReportStr);

						wString_sprintf(&g_StateMachineWorker.m_Stats.m_ReportStr, "Main: %" PRIu32 "/s                   \r\n", g_StateMachineWorker.m_Stats.m_MainWhile);

						if(g_StateMachineWorker.m_Stats.m_MainWhileAvg == 0)
							wString_sprintf(&g_StateMachineWorker.m_Stats.m_ReportStr, "Avg: ?/s                   \r\n");
						else
							wString_sprintf(&g_StateMachineWorker.m_Stats.m_ReportStr, "Avg: %" PRIu32 "/s                   \r\n", g_StateMachineWorker.m_Stats.m_MainWhileAvg);


						#ifdef STATEMACHINE_ENABLE_THROTTLE
							if(g_StateMachineWorker.m_Throttle.m_Enabled == True)
							{
								wString_sprintf(&g_StateMachineWorker.m_Stats.m_ReportStr, "Throttling: %i (%2.3fms)\r\n", g_StateMachineWorker.m_Throttle.m_Target, ((double)g_StateMachineWorker.m_Throttle.us) / 1000.0);
							}
							else
							{
								wString_Put(&g_StateMachineWorker.m_Stats.m_ReportStr, "Throttling: OFF\r\n");
							}
						#endif
						wString_sprintf(&g_StateMachineWorker.m_Stats.m_ReportStr, "---------------- Tasks (%" PRIu32 "/s) ----------------\r\n", g_StateMachineWorker.m_Stats.m_IterationsPerSec);
						wString_sprintf(&g_StateMachineWorker.m_Stats.m_ReportStr, "ID\tACT\tAVG (60s)\tMAX (60s)\tNAME\r\n");

						float total = 0.0f;
						float avgTotal = 0.0f;
						float maxTotal = 0.0f;
						node = g_StateMachineWorker.m_List.m_HeadNode;
						while(node != NULL)
						{
							StateMachineWorker_Task* _Task = (StateMachineWorker_Task*)node->m_Item;
							if(_Task->m_Clock > 0)
								_Task->m_CPU = ((float)_Task->m_Clock / (float)g_StateMachineWorker.m_Stats.m_TotalClock) * 100;
							else
								_Task->m_CPU = 0;

							total += _Task->m_CPU;

							if(_Task->m_CPU > _Task->m_CurrMAX)
								_Task->m_CurrMAX = _Task->m_CPU;

							if(_Task->m_CPU > _Task->m_MAX)
								_Task->m_MAX = _Task->m_CPU;

							maxTotal += _Task->m_MAX;
							_Task->m_AverageValue += _Task->m_CPU;
							_Task->m_AverageCount++;

							avgTotal += _Task->m_AVG;

							if(_Task->m_AverageCount >= 60)
							{
								_Task->m_AVG = _Task->m_AverageValue / (float)_Task->m_AverageCount;
								_Task->m_AverageValue = 0.0f;
								_Task->m_AverageCount = 0;
								_Task->m_MAX = _Task->m_CurrMAX;
								_Task->m_CurrMAX = _Task->m_CPU;
							}

							wString_sprintf(&g_StateMachineWorker.m_Stats.m_ReportStr, "T%03" PRIu32 ":\t%05.2f%%\t%05.2f%%\t\t%05.2f%%\t\t%s\r\n", _Task->m_tID, _Task->m_CPU, _Task->m_AVG, _Task->m_MAX, _Task->m_Name.size == 0 ? "NoName" : _Task->m_Name.str);

							_Task->m_Clock = 0;
							node = node->m_NextNode;
						}

						if(g_StateMachineWorker.m_Stats.m_Visual == True) {
							wString_sprintf(&g_StateMachineWorker.m_Stats.m_ReportStr, "     \t%05.2f%%\t%05.2f%%\t\t%05.2f%%\r\n", total, avgTotal, maxTotal);
							wString_sprintf(&g_StateMachineWorker.m_Stats.m_ReportStr, "----------------------------------------------\r\n");

							system("clear");
							printf("%s\r\n", g_StateMachineWorker.m_Stats.m_ReportStr.str);
						}

						g_StateMachineWorker.m_Stats.m_TotalClock = 0;
						g_StateMachineWorker.m_Stats.m_Timeout = SystemMonotonic() + STATEMACHINE_ENABLE_STATS_SAMPLE_RATE;
					}
				}
			}
		#endif
	}

	return monTime;
}

StateMachineWorker_Task* StateMachineWorker_CreateTask(void* _Object, void (*_FuncPtr)(StateMachineWorker_Task* _Task, UInt64 _MonTime), const char* _Name, const char* _SubName)
{
	StateMachineWorker_Task* newTask = (StateMachineWorker_Task*)WAllocator_Alloc(sizeof(StateMachineWorker_Task));
	if(newTask == NULL)
		return NULL;

	newTask->m_Delete = False;
	newTask->m_Removed = False;
	newTask->m_Object = _Object;
	newTask->m_Context = _Object;
	newTask->m_FuncPtr = _FuncPtr;
	newTask->m_Suspended = 0;
	newTask->m_SuspendUntil = 0;

	#ifdef STATEMACHINE_ENABLE_STATS
		newTask->m_Clock = 0;
		newTask->m_CPU = 0.0f;
		newTask->m_MAX = 0.0f;
		newTask->m_CurrMAX = 0.0f;

		newTask->m_AVG = 0.0f;
		newTask->m_AverageValue = 0.0f;
		newTask->m_AverageCount = 0;

		newTask->m_tID = g_StateMachineWorker.m_tIDUbound++;

		int size = strlen(_Name);
		wString_Initialize(&newTask->m_Name, size + 4);
		wString_Set(&newTask->m_Name, _Name);

		if(_SubName != NULL)
		{
			if(strlen(_SubName) > 0)
				wString_sprintf(&newTask->m_Name, "/%s", _SubName);

		}
	#endif

	if(LinkedList_Push(&g_StateMachineWorker.m_List, newTask) != 0)
	{
		#ifdef STATEMACHINE_ENABLE_STATS
			wString_Dispose(&newTask->m_Name);
		#endif
		WAllocator_Free((unsigned char*)newTask);
		return NULL;

	}
	return newTask;
}

StateMachineWorker_Task* StateMachineWorker_CreatePrio(void* _Object, void (*_FuncPtr)(StateMachineWorker_Task* _Task, UInt64 _MonTime))
{
	StateMachineWorker_Task* newTask = (StateMachineWorker_Task*)WAllocator_Alloc(sizeof(StateMachineWorker_Task));
	if(newTask == NULL)
		return NULL;

	newTask->m_Delete = False;
	newTask->m_Removed = False;
	newTask->m_Object = _Object;
	newTask->m_Context = _Object;
	newTask->m_FuncPtr = _FuncPtr;
	newTask->m_Suspended = 0;

	#ifdef STATEMACHINE_ENABLE_STATS
		newTask->m_Clock = 0;
		newTask->m_tID = g_StateMachineWorker.m_tIDUbound++;

		wString_Initialize(&newTask->m_Name,4);
	#endif

	if(LinkedList_Push(&g_StateMachineWorker.m_PrioList, newTask) != 0)
	{
		#ifdef STATEMACHINE_ENABLE_STATS
			wString_Dispose(&newTask->m_Name);
		#endif
		WAllocator_Free((unsigned char*)newTask);
		return NULL;

	}
	return newTask;
}

void StateMachineWorker_SuspendTask(StateMachineWorker_Task* _Task, UInt64 _AutoResume)
{
	if(_AutoResume > 0)
	{
		UInt64 monTime;
		SystemMonotonicMS(&monTime);
		monTime += _AutoResume;
		if(_Task->m_Suspended != monTime)
		{
			_Task->m_Suspended = monTime;

			#ifdef STATEMACHINE_ENABLE_STATS
				//printf("StateMachineWorker_Task(%s) is suspended for %" PRIu64 " seconds!\r\n", _Task->m_Name.str, (_AutoResume / 1000));
			#endif
		}
		return;
	}

	if(_Task->m_Suspended != UINT64_MAX)
	{
		_Task->m_Suspended = UINT64_MAX;

		#ifdef STATEMACHINE_ENABLE_STATS
			//printf("StateMachineWorker_Task(%s) is suspended indefinitely!\r\n", _Task->m_Name.str);
		#endif
	}
}

void StateMachineWorker_SuspendTaskUntil(StateMachineWorker_Task* _Task, UInt64 _UTC)
{
	_Task->m_SuspendUntil = _UTC;
	_Task->m_Suspended = UINT64_MAX;
}

void StateMachineWorker_ResumeTask(StateMachineWorker_Task* _Task)
{
	if(_Task->m_Suspended != 0)
	{
		_Task->m_Suspended = 0;
		_Task->m_SuspendUntil = 0;

		#ifdef STATEMACHINE_ENABLE_STATS
			//printf("StateMachineWorker_Task(%s) is resumed!\r\n", _Task->m_Name.str);
		#endif
	}
}

void StateMachineWorker_DeleteTask(StateMachineWorker_Task* _Task)
{
	_Task->m_Delete = True;
}

void StateMachineWorker_Abort(StateMachineWorker_Task* _Task)
{
	_Task->m_Delete = True;
}

void StateMachineWorker_Dispose()
{

	#ifdef STATEMACHINE_ENABLE_STATS
		wString_Dispose(&g_StateMachineWorker.m_Stats.m_ReportStr);
	#endif

	LinkedList_Node* node = g_StateMachineWorker.m_List.m_HeadNode;
	while(node != NULL)
	{
		StateMachineWorker_Task* _Task = (StateMachineWorker_Task*)node->m_Item;
		node = node->m_NextNode;

		LinkedList_Remove(&g_StateMachineWorker.m_List, _Task);
		#ifdef STATEMACHINE_ENABLE_STATS
			wString_Dispose(&_Task->m_Name);
		#endif
		WAllocator_Free((unsigned char*)_Task);
	}

	LinkedList_Dispose(&g_StateMachineWorker.m_List);

	node = g_StateMachineWorker.m_PrioList.m_HeadNode;
	while(node != NULL)
	{
		StateMachineWorker_Task* _Task = (StateMachineWorker_Task*)node->m_Item;
		node = node->m_NextNode;

		LinkedList_Remove(&g_StateMachineWorker.m_PrioList, _Task);
		#ifdef STATEMACHINE_ENABLE_STATS
			wString_Dispose(&_Task->m_Name);
		#endif
		WAllocator_Free((unsigned char*)_Task);
	}

	LinkedList_Dispose(&g_StateMachineWorker.m_PrioList);

	g_StateMachineWorker.m_Disposed = True;
}

