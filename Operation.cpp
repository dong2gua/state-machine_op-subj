#include "Operation.h"
#include "Subject.h"
#include <iostream>
#include <thread>
#include <chrono>

Operation::Operation(int id) : StateMachine(ST_MAX_STATES, ST_IDLE), pDoer(nullptr), _isAborting(false)
{
	this->_id = id;
}

Operation::~Operation()
{
}

int Operation::GetId()
{
	return this->_id;
}

STATE_DEFINE(Operation, Idle, NoEventData)
{
	std::cout << "Operation::ST_Idle" << std::endl;
	// Release subject
	this->pDoer->Release();
	this->pDoer = nullptr;
}

STATE_DEFINE(Operation, Frozen, EventData)
{
	std::cout << "Operation::ST_Frozen" << std::endl;
	auto paramData = dynamic_cast<const ParameterData *>(data);
	if (paramData != nullptr)
	{
		this->candidateParameter = paramData->pseudoData;
		this->actionParameter = this->candidateParameter;
		std::cout << "Operation has frozen with parameter: " << this->actionParameter.value << std::endl;
	}
	else
	{
		auto subjData = dynamic_cast<const SubjectData *>(data);
		if (subjData != nullptr)
		{
			this->actionParameter = this->candidateParameter;
			std::cout << "Operation has frozen with parameter: " << this->actionParameter.value << std::endl;
			// A new event parameter must be created for next state, the current data will be deleted after calling the state function.
			SubjectData *pSubData = new SubjectData();
			pSubData->TargetSubject = subjData->TargetSubject;
			InternalEvent(ST_WAIT, pSubData);
		}
	}
}

STATE_DEFINE(Operation, Wait, SubjectData)
{
	std::cout << "Operation::ST_Wait" << std::endl;
	this->pDoer = data->TargetSubject;
	std::cout << "The operation is waiting for subject: " << this->pDoer->GetId() << std::endl;
	// Send interrupt signal to the operation which is ongoing.
	this->pDoer->AbortCurrentOperation();
	// Wait until the subject become idle.
	this->pDoer->WaitForIdle();
	InternalEvent(ST_RUN);
}

ENTRY_DEFINE(Operation, EntryRun, NoEventData)
{
	// Acquire the subject before start doing any action.
	OperationData *opData = new OperationData();
	opData->pOperation = this;
	this->pDoer->Acquire(opData);
}

STATE_DEFINE(Operation, Run, NoEventData)
{
	std::cout << "Operation::ST_Run" << std::endl;
	std::cout << "Start the action on the subject" << this->pDoer->GetId() << std::endl;
	// start the action on the subject.
	std::thread workingThread(&Operation::WorkToDo, this);
	workingThread.detach();
}

STATE_DEFINE(Operation, Complete, NoEventData)
{
	std::cout << "Operation::ST_Complete" << std::endl;
	// Operation state back to ST_IDLE
	InternalEvent(ST_IDLE);
}

STATE_DEFINE(Operation, Abort, NoEventData)
{
	std::cout << "Operation::ST_Abort" << std::endl;
	// Release subject
	// Operation state back to ST_IDLE
	// synchronous call
	InternalEvent(ST_IDLE);
}

// The current state remains the old.
ENTRY_DEFINE(Operation, EntryAbort, NoEventData)
{
	// abort current action in blocking mode.
	this->_isAborting = true;
	// Wait until aborted..
	WaitUntilAborted();
	this->_isAborting = false;
}

void Operation::Commit(ParameterData *param)
{
	BEGIN_TRANSITION_MAP					// - Current State -
	TRANSITION_MAP_ENTRY(ST_FROZEN)			// ST_IDLE
		TRANSITION_MAP_ENTRY(ST_FROZEN)		// ST_FROZEN
		TRANSITION_MAP_ENTRY(CANNOT_HAPPEN) // ST_WAIT
		TRANSITION_MAP_ENTRY(CANNOT_HAPPEN) // ST_RUN
		TRANSITION_MAP_ENTRY(CANNOT_HAPPEN) // ST_COMPLETE
		TRANSITION_MAP_ENTRY(CANNOT_HAPPEN) // ST_ABORT
		END_TRANSITION_MAP(param)
}

void Operation::Start(SubjectData *param)
{
	BEGIN_TRANSITION_MAP					// - Current State -
	TRANSITION_MAP_ENTRY(ST_FROZEN)			// ST_IDLE
		TRANSITION_MAP_ENTRY(ST_WAIT)		// ST_FROZEN
		TRANSITION_MAP_ENTRY(CANNOT_HAPPEN) // ST_WAIT
		TRANSITION_MAP_ENTRY(CANNOT_HAPPEN) // ST_RUN
		TRANSITION_MAP_ENTRY(CANNOT_HAPPEN) // ST_COMPLETE
		TRANSITION_MAP_ENTRY(CANNOT_HAPPEN) // ST_ABORT
		END_TRANSITION_MAP(param)
}

void Operation::Interrupt()
{
	BEGIN_TRANSITION_MAP					// - Current State -
	TRANSITION_MAP_ENTRY(CANNOT_HAPPEN)		// ST_IDLE
		TRANSITION_MAP_ENTRY(CANNOT_HAPPEN) // ST_FROZEN
		TRANSITION_MAP_ENTRY(CANNOT_HAPPEN) // ST_WAIT
		TRANSITION_MAP_ENTRY(ST_ABORT)		// ST_RUN
		TRANSITION_MAP_ENTRY(CANNOT_HAPPEN) // ST_COMPLETE
		TRANSITION_MAP_ENTRY(CANNOT_HAPPEN) // ST_ABORT
		END_TRANSITION_MAP(nullptr)
}

void Operation::GetDABAStatus(bool *isDone, bool *isAborted, bool *isBusy, bool *isActive)
{
	States currentState = static_cast<States>(GetCurrentState());
	short chState = 0x0;
	switch (currentState)
	{
	case ST_IDLE:
	case ST_FROZEN:
		chState = 0x0000;
		break;
	case ST_WAIT:
		chState = 0x0010;
		break;
	case ST_RUN:
		chState = 0x0011;
		break;
	case ST_COMPLETE:
		chState = 0x1000;
		break;
	case ST_ABORT:
		chState = 0x0100;
		break;
	default:
		break;
	}
	*isDone = static_cast<bool>(chState & 0x1000);
	*isAborted = static_cast<bool>(chState & 0x0100);
	*isBusy = static_cast<bool>(chState & 0x0010);
	*isActive = static_cast<bool>(chState & 0x001);

	std::cout << "Operation [" << this->GetId() << "] status: "
			  << "Done-" << *isDone << " Aborted-" << *isAborted << " Busy-" << *isBusy << " Active-" << *isActive << std::endl;
}

void Operation::WaitUntilAborted()
{
	std::unique_lock<std::mutex> lk(mut_WorkThread);
	std::cout << "Operation " << this->_id << " wait until the work aborted." << std::endl;
	cv_WorkThread.wait(lk);
	std::cout << "Operation " << this->_id << " aborted." << std::endl;
}

void Operation::WorkerStoppedSignal()
{
	{
		std::lock_guard<std::mutex> lk(mut_WorkThread);
	}
	cv_WorkThread.notify_one();
}

void Operation::WorkToDo()
{
	std::cout << "Operation " << this->_id << " start working...." << std::endl;
	// take total 10s to complete
	for (auto i = 0; i < 10; i++)
	{
		std::cout << "Operation " << this->_id << " work in progress: " << i + 1 << std::endl;
		if (this->_isAborting)
		{
			break;
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	if (!this->_isAborting)
	{
		ExternalEvent(ST_COMPLETE);
	}
	WorkerStoppedSignal();
}
