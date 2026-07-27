#include "PJ_Quiet_Protocol/GameMode/QPEscapeGameState.h"
#include "Net/UnrealNetwork.h"

AQPEscapeGameState::AQPEscapeGameState()
{
	RemainingGenerators = 0;
	TotalGenerators = 0;
}

void AQPEscapeGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AQPEscapeGameState, RemainingGenerators);
	DOREPLIFETIME(AQPEscapeGameState, TotalGenerators);
}
