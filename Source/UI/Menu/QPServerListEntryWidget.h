#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PJ_Quiet_Protocol/Session/QPSessionSubsystem.h"
#include "QPServerListEntryWidget.generated.h"

class UTextBlock;
class UButton;

/**
 * 방 목록 한 줄을 표시하는 엔트리 위젯
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API UQPServerListEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* HostNameText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* PingText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* PlayerCountText;

	UPROPERTY(meta = (BindWidget))
	UButton* JoinButton;

	UFUNCTION(BlueprintCallable, Category = "QP|UI|Menu")
	void Setup(UQPSessionSubsystem* InSubsystem, const FQPBlueprintSessionInfo& SessionInfo);

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnJoinButtonClicked();

private:
	UPROPERTY()
	UQPSessionSubsystem* SessionSubsystem;

	int32 SessionIndex;
};
