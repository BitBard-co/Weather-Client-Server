// Include the header that declares types and constants for this module (smw_task, smw_max_tasks, etc.)
#include "smw.h"
// Include C standard string functions used here (memset)
#include <string.h>
// Global state for the simple mini worker (task registry and storage)
smw g_smw;
// Initialize the global task registry; returns 0 on success
int smw_init()
{
	// Zero-out the entire global struct to a known clean state
	memset(&g_smw, 0, sizeof(g_smw));
	// Loop index for iterating tasks
	int i;
	// Iterate through all task slots to explicitly clear pointers
	for(i = 0; i < smw_max_tasks; i++)
	{
		// No user context associated yet
		g_smw.tasks[i].context = NULL;
		// No callback function registered yet
		g_smw.tasks[i].callback = NULL;
	}
	// Signal success
	return 0;
}
// Create/register a task: store a user context and a callback taking (context, monotonic time)
smw_task* smw_createTask(void* _Context, void (*_Callback)(void* _Context, uint64_t _MonTime))
{
	// Loop index for searching a free slot
	int i;
	// Find the first free slot where both pointers are NULL
	for(i = 0; i < smw_max_tasks; i++)
	{
		// Free slot detected (uninitialized task entry)
		if(g_smw.tasks[i].context == NULL && g_smw.tasks[i].callback == NULL)
		{
			// Assign user-provided context pointer
			g_smw.tasks[i].context = _Context;
			// Assign user-provided callback function pointer
			g_smw.tasks[i].callback = _Callback;
			// Return the address of the newly created task entry
			return &g_smw.tasks[i];
		}
	}
	// No free slots available; signal failure
	return NULL;
}
// Destroy/unregister a previously created task by clearing its entry
void smw_destroyTask(smw_task* _Task)
{
	// Ignore null input; nothing to do
	if(_Task == NULL)
		return;
	// Loop index for locating the given task within the registry
	int i;
	// Search for the matching task slot by pointer comparison
	for(i = 0; i < smw_max_tasks; i++)
	{
		// Found the exact task entry corresponding to the passed pointer
		if(&g_smw.tasks[i] == _Task)
		{
			// Clear the user context to mark slot as free
			g_smw.tasks[i].context = NULL;
			// Clear the callback pointer to mark slot as free
			g_smw.tasks[i].callback = NULL;
			// Stop searching after clearing the matching entry
			break;
		}
	}
}
// Execute all registered tasks by invoking their callbacks with the given monotonic time
void smw_work(uint64_t _MonTime)
{
	// Loop index for iterating over all task entries
	int i;
	// Iterate through task slots to run active callbacks
	for(i = 0; i < smw_max_tasks; i++)
	{
		// Only invoke tasks that have a valid callback function
		if(g_smw.tasks[i].callback != NULL)
			// Call the task's function with its context and the provided time stamp
			g_smw.tasks[i].callback(g_smw.tasks[i].context, _MonTime);
		// (No else: empty slots are skipped)
	}
}
// Count how many tasks are currently registered (i.e., have a non-NULL callback)
int smw_getTaskCount()
{
	// Number of active tasks found
	int counter = 0;
	// Loop index for iterating task entries
	int i;
	// Scan all slots and increment for each active callback
	for(i = 0; i < smw_max_tasks; i++)
	{
		// A non-NULL callback indicates an active task
		if(g_smw.tasks[i].callback != NULL)
			counter++;
		// Continue scanning remaining slots
	}
	// Return the number of active tasks
	return counter;
}
// Clear all tasks and dispose the registry (makes every slot free)
void smw_dispose()
{
	// Loop index for clearing each task slot
	int i;
	// Iterate through all slots and reset pointers to NULL
	for(i = 0; i < smw_max_tasks; i++)
	{
		// Remove any stored user context
		g_smw.tasks[i].context = NULL;
		// Remove any stored callback function
		g_smw.tasks[i].callback = NULL;
	}
}
