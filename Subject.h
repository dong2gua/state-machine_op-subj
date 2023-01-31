#pragma once
#include "StateMachine.h"
#include <mutex>
#include <condition_variable>

class Operation;

class OperationData : public EventData
{
public:
	Operation *pOperation;
};

class Subject : public StateMachine
{

public:
	Subject(int id);
	virtual ~Subject();
	int GetId();

	// External signal
	void Acquire(OperationData *param);
	void Release();

	void AbortCurrentOperation();
	void WaitForIdle();

private:
	int _id;
	Operation *_pCurrentOperation;

	std::mutex mut_IdleSignal;
	std::condition_variable cv_IdleSignal;

	enum States
	{
		ST_IDLE,
		ST_OCCUPIED,
		ST_MAX_STATES
	};

	STATE_DECLARE(Subject, Idle, NoEventData)
	STATE_DECLARE(Subject, Occupied, OperationData)

	BEGIN_STATE_MAP
	STATE_MAP_ENTRY(&Idle)
	STATE_MAP_ENTRY(&Occupied)
	END_STATE_MAP
};
