#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "QPAnimNotify_InsertShell.generated.h"

UCLASS()
class PJ_QUIET_PROTOCOL_API UQPAnimNotify_InsertShell : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
