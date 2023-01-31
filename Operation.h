#pragma once
#include "StateMachine.h"
#include <mutex>
#include <condition_variable>

struct PseudoData
{
	int value;
};

class ParameterData : public EventData
{
public:
	virtual ~ParameterData(){};
	PseudoData pseudoData;
};

class Subject;
class SubjectData : public EventData
{
public:
	virtual ~SubjectData(){};
	Subject *TargetSubject;
};

class Operation : public StateMachine
{
public:
	Operation(int id);
	virtual ~Operation();
	int GetId();
	// External Event
	void Commit(ParameterData *param);
	void Start(SubjectData *param);
	void Interrupt();

	void GetDABAStatus(bool *isDone, bool *isAborted, bool *isBusy, bool *isActive);
	PseudoData candidateParameter;

private:
	int _id;
	Subject *pDoer;
	PseudoData actionParameter;
	enum States
	{
		ST_IDLE,
		ST_FROZEN,
		ST_WAIT,
		ST_RUN,
		ST_COMPLETE,
		ST_ABORT,
		ST_MAX_STATES
	};

	STATE_DECLARE(Operation, Idle, NoEventData)
	STATE_DECLARE(Operation, Frozen, EventData)
	STATE_DECLARE(Operation, Wait, SubjectData)
	ENTRY_DECLARE(Operation, EntryRun, NoEventData)
	STATE_DECLARE(Operation, Run, NoEventData)
	STATE_DECLARE(Operation, Complete, NoEventData)
	STATE_DECLARE(Operation, Abort, NoEventData)
	ENTRY_DECLARE(Operation, EntryAbort, NoEventData)

	BEGIN_STATE_MAP_EX
	STATE_MAP_ENTRY_EX(&Idle)
	STATE_MAP_ENTRY_EX(&Frozen)
	STATE_MAP_ENTRY_EX(&Wait)
	STATE_MAP_ENTRY_ALL_EX(&Run, nullptr, &EntryRun, nullptr)
	STATE_MAP_ENTRY_EX(&Complete)
	STATE_MAP_ENTRY_ALL_EX(&Abort, nullptr, &EntryAbort, nullptr)
	END_STATE_MAP_EX

	bool _isAborting;
	void WaitUntilAborted();
	void WorkToDo();
	void WorkerStoppedSignal();

	std::mutex mut_WorkThread;
	std::condition_variable cv_WorkThread;
};
