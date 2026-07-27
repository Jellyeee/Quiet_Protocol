#include "QPAnimNotify_InsertShell.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
#include "PJ_Quiet_Protocol/Character/Components/QPCombatComponent.h"

void UQPAnimNotify_InsertShell::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (AQPCharacter* Character = Cast<AQPCharacter>(MeshComp->GetOwner()))
		{
			if (UQPCombatComponent* CombatComponent = Character->GetCombatComponent())
			{
				CombatComponent->InsertShotgunShell();
			}
		}
	} // end
}
