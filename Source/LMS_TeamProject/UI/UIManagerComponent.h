#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/SlateWrapperTypes.h"
#include "UIManagerComponent.generated.h"

// LMS 프로젝트에서 사용할 UI 종류를 구분하는 enum입니다.
// UIManagerComponent는 이 enum 값을 기준으로 UI를 생성, 표시, 숨김 처리합니다.
UENUM(BlueprintType)
enum class ELMSUIType : uint8
{
	Main	UMETA(DisplayName = "Main UI"),
	Combat	UMETA(DisplayName = "Combat UI"),
	Pause	UMETA(DisplayName = "Pause UI"),
	Respawn	UMETA(DisplayName = "Respawn UI"),
	Lobby	UMETA(DisplayName = "Lobby UI"),
	Result	UMETA(DisplayName = "Result UI"),
	Indicator UMETA(DisplayName = "Indicator Overlay UI"),
	WeaponSelect UMETA(DisplayName = "Weapon Select UI")

};

// UI 하나를 생성하고 표시하기 위한 설정 데이터입니다.
// 에디터에서 UIType별 WidgetClass, ZOrder, Visibility 등을 등록할 때 사용합니다.
USTRUCT(BlueprintType)
struct FLMSUIData
{
	GENERATED_BODY()

	// 이 데이터가 어떤 UI를 의미하는지 구분합니다.
	// 예: Main, Combat, Pause, Result
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LMS|UI")
	ELMSUIType UIType = ELMSUIType::Main;

	// 실제로 생성할 Widget Blueprint 클래스입니다.
	// 예: WBP_CombatHUD_Test
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LMS|UI")
	TSubclassOf<UUserWidget> WidgetClass;

	// 화면에 표시될 순서입니다.
	// 숫자가 클수록 더 앞쪽에 표시됩니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LMS|UI")
	int32 ZOrder = 0;

	// ShowUI 호출 시 적용할 Visibility 상태입니다.
	// 전투 HUD처럼 마우스 클릭을 막으면 안 되는 UI는 Not Hit-Testable 계열을 권장합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LMS|UI")
	ESlateVisibility VisibleState = ESlateVisibility::Visible;

	// BeginPlay 시점에 미리 Widget을 생성할지 여부입니다.
	// false면 ShowUI가 처음 호출될 때 생성됩니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LMS|UI")
	bool bCreateOnBeginPlay = false;

	// BeginPlay 시점에 바로 화면에 표시할지 여부입니다.
	// true면 BeginPlay에서 ShowUI가 호출됩니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LMS|UI")
	bool bShowOnBeginPlay = false;
};

// 여러 UI를 생성/표시/숨김 처리하는 관리자 컴포넌트입니다.
// 현재 구조에서는 PlayerController에 붙여서 사용합니다.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LMS_TEAMPROJECT_API UUIManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUIManagerComponent();

protected:
	virtual void BeginPlay() override;

public:
	// 지정한 UIType에 해당하는 Widget을 생성합니다.
	// 이미 생성되어 있으면 새로 만들지 않고 기존 Widget을 반환합니다.
	UFUNCTION(BlueprintCallable, Category = "LMS|UI")
	UUserWidget* CreateUI(ELMSUIType UIType);

	// 지정한 UIType에 해당하는 Widget을 화면에 표시합니다.
	// 아직 생성되지 않았다면 CreateUI를 통해 먼저 생성합니다.
	UFUNCTION(BlueprintCallable, Category = "LMS|UI")
	void ShowUI(ELMSUIType UIType);

	// 지정한 UIType에 해당하는 Widget을 숨깁니다.
	// 화면에서 제거하지 않고 Collapsed 상태로 숨깁니다.
	UFUNCTION(BlueprintCallable, Category = "LMS|UI")
	void HideUI(ELMSUIType UIType);

	// 지정한 UIType에 해당하는 Widget을 켜고 끕니다.
	// 보이는 상태면 숨기고, 숨겨진 상태면 표시합니다.
	UFUNCTION(BlueprintCallable, Category = "LMS|UI")
	void ToggleUI(ELMSUIType UIType);

	// 생성된 모든 UI를 숨깁니다.
	// 일시정지, 결과창 전환 등에서 사용할 수 있습니다.
	UFUNCTION(BlueprintCallable, Category = "LMS|UI")
	void HideAllUI();

	// 지정한 UIType에 해당하는 생성된 Widget을 반환합니다.
	// 아직 생성되지 않았다면 nullptr을 반환합니다.
	UFUNCTION(BlueprintCallable, Category = "LMS|UI")
	UUserWidget* GetUI(ELMSUIType UIType) const;

private:
	// 에디터에서 등록할 UI 목록입니다.
	// Combat, Main, Pause 같은 UI 타입별 WidgetClass와 표시 옵션을 설정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LMS|UI", meta = (AllowPrivateAccess = "true"))
	TArray<FLMSUIData> UIDataList;

	// 게임 중 실제로 생성된 UI Widget들을 보관합니다.
	// 같은 UI가 중복 생성되지 않도록 UIType을 Key로 사용합니다.
	UPROPERTY()
	TMap<ELMSUIType, TObjectPtr<UUserWidget>> CreatedUIMap;

private:
	// UIDataList에서 지정한 UIType에 해당하는 설정 데이터를 찾습니다.
	// 찾지 못하면 nullptr을 반환합니다.
	const FLMSUIData* FindUIData(ELMSUIType UIType) const;
};