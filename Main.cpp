#include "Operation.h"
#include "Subject.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace std;

// For the definition and detailed transition map, please refer to:
// https://sioux.atlassian.net/wiki/spaces/2/pages/2259746831/State+Transition+Table+for+AxisResource+ReadMethodStatus

void TestOperationOnSubject()
{
	bool done, aborted, busy, active;

	Operation op1(111);
	Operation op2(222);
	Subject sub(300);

	PseudoData testParamData;
	testParamData.value = 654;
	ParameterData *pParam = new ParameterData();
	pParam->pseudoData = testParamData;

	op1.Commit(pParam);

	// op1.candidateParameter.value = 888;
	SubjectData *pSubData = new SubjectData();
	pSubData->TargetSubject = &sub;
	op1.Start(pSubData);

	// Wait for a while so that the operation can do some work.
	std::this_thread::sleep_for(std::chrono::seconds(3));
	// op1.Interrupt();

	// simulate another operation is acquiring the subject.
	PseudoData testParamData2;
	testParamData2.value = 789;
	ParameterData *pParam2 = new ParameterData();
	pParam2->pseudoData = testParamData2;
	op2.Commit(pParam2);
	SubjectData *pSubData2 = new SubjectData();
	pSubData2->TargetSubject = &sub;
	op2.Start(pSubData2);

	// Press any key to exit.
	int c = getchar();
}

int main(void)
{
	TestOperationOnSubject();
}
