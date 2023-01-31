#include "Subject.h"
#include "Operation.h"
#include <iostream>

Subject::Subject(int id) : StateMachine(ST_MAX_STATES, ST_IDLE), _pCurrentOperation(nullptr)
{
	this->_id = id;
}

Subject::~Subject()
{
}

int Subject::GetId()
{
	return this->_id;
}

STATE_DEFINE(Subject, Idle, NoEventData)
{
	std::cout << "Subject::ST_IDLE" << std::endl;
	std::cout << "Subject is in idle state." << std::endl;
	this->_pCurrentOperation = nullptr;
	// issue a signal
	{
		std::lock_guard<std::mutex> lk(mut_IdleSignal);
	}
	cv_IdleSignal.notify_one();
}

STATE_DEFINE(Subject, Occupied, OperationData)
{
	std::cout << "Subject::ST_OCCUPIED" << std::endl;
	this->_pCurrentOperation = data->pOperation;
	std::cout << "Subject is occupied by operation [Id]:" << data->pOperation->GetId() << std::endl;
}

void Subject::Acquire(OperationData *param)
{
	BEGIN_TRANSITION_MAP					// - Current State -
	TRANSITION_MAP_ENTRY(ST_OCCUPIED)		// ST_IDLE
		TRANSITION_MAP_ENTRY(EVENT_IGNORED) // ST_OCCUPIED
		END_TRANSITION_MAP(param)
}

void Subject::Release()
{
	BEGIN_TRANSITION_MAP				// - Current State -
	TRANSITION_MAP_ENTRY(EVENT_IGNORED) // ST_IDLE
		TRANSITION_MAP_ENTRY(ST_IDLE)	// ST_OCCUPIED
		END_TRANSITION_MAP(nullptr)
}

void Subject::AbortCurrentOperation()
{
	if (this->_pCurrentOperation != nullptr)
	{
		this->_pCurrentOperation->Interrupt();
	}
}

void Subject::WaitForIdle()
{
	States currentState = static_cast<States>(GetCurrentState());
	if (currentState == ST_IDLE)
		return;

	std::unique_lock<std::mutex> lk(mut_IdleSignal);
	std::cout << "Subject " << this->_id << " wait until idle." << std::endl;
	cv_IdleSignal.wait(lk);
	std::cout << "Subject " << this->_id << " is idle." << std::endl;
}
