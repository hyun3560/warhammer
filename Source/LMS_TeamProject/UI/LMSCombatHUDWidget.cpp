// Fill out your copyright notice in the Description page of Project Settings.


#include "LMSCombatHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void ULMSCombatHUDWidget::SetWeaponInfo(UTexture2D* WeaponIcon, bool bShowAmmo)
{
	if (!WidgetTree)
	{
		return;
	}

	if (UImage* WeaponIconImage = WidgetTree->FindWidget<UImage>(TEXT("IMG_WeaponIcon")))
	{
		if (WeaponIcon)
		{
			WeaponIconImage->SetBrushFromTexture(WeaponIcon, true);
		}
	}

	const ESlateVisibility AmmoVisibility = bShowAmmo
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Hidden;

	if (UTextBlock* CurrentAmmoText = WidgetTree->FindWidget<UTextBlock>(TEXT("TXT_CurrentAmmo")))
	{
		CurrentAmmoText->SetVisibility(AmmoVisibility);
	}

	if (UTextBlock* ReserveAmmoText = WidgetTree->FindWidget<UTextBlock>(TEXT("TXT_ReserveAmmo")))
	{
		ReserveAmmoText->SetVisibility(AmmoVisibility);
	}
}
