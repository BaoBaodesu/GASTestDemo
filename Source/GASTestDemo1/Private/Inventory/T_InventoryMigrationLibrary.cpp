#include "Inventory/T_InventoryMigrationLibrary.h"

#if WITH_EDITOR
#include "Engine/Blueprint.h"
#include "Blueprint/UserWidget.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Serialization/ArchiveReplaceObjectRef.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Animation/WidgetAnimation.h"
#include "Components/PanelWidget.h"
#include "Components/PanelSlot.h"
#include "UObject/UObjectHash.h"
#include "UObject/UnrealType.h"
#endif

#if WITH_EDITOR
namespace
{
	void NormalizeWidgetVariableGuids(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!IsValid(WidgetBlueprint) || !IsValid(WidgetBlueprint->WidgetTree)) return;

		TSet<FName> VariableNames;
		WidgetBlueprint->ForEachSourceWidget([WidgetBlueprint, &VariableNames](UWidget* Widget)
		{
			if (!IsValid(Widget)) return;
			VariableNames.Add(Widget->GetFName());
			if (!WidgetBlueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()))
			{
				WidgetBlueprint->WidgetVariableNameToGuidMap.Add(Widget->GetFName(), FGuid::NewGuid());
			}
		});
		for (UWidgetAnimation* Animation : WidgetBlueprint->Animations)
		{
			if (!IsValid(Animation)) continue;
			VariableNames.Add(Animation->GetFName());
			if (!WidgetBlueprint->WidgetVariableNameToGuidMap.Contains(Animation->GetFName()))
			{
				WidgetBlueprint->WidgetVariableNameToGuidMap.Add(Animation->GetFName(), FGuid::NewGuid());
			}
		}

		for (auto It = WidgetBlueprint->WidgetVariableNameToGuidMap.CreateIterator(); It; ++It)
		{
			if (!VariableNames.Contains(It.Key())) It.RemoveCurrent();
		}
	}
}
#endif

bool UT_InventoryMigrationLibrary::CleanAndReparentBlueprint(UBlueprint* Blueprint, UClass* NewParentClass)
{
#if WITH_EDITOR
	if (!IsValid(Blueprint) || !IsValid(NewParentClass)) return false;

	while (!Blueprint->ImplementedInterfaces.IsEmpty())
	{
		const UClass* InterfaceClass = Blueprint->ImplementedInterfaces.Last().Interface;
		if (IsValid(InterfaceClass)) FBlueprintEditorUtils::RemoveInterface(Blueprint, InterfaceClass->GetClassPathName(), false);
		else Blueprint->ImplementedInterfaces.Pop();
	}

	TArray<TObjectPtr<UEdGraph>> Graphs;
	Graphs.Append(Blueprint->UbergraphPages);
	Graphs.Append(Blueprint->FunctionGraphs);
	Graphs.Append(Blueprint->MacroGraphs);
	Graphs.Append(Blueprint->DelegateSignatureGraphs);
	for (UEdGraph* Graph : Graphs)
	{
		if (IsValid(Graph)) FBlueprintEditorUtils::RemoveGraph(Blueprint, Graph, EGraphRemoveFlags::MarkTransient);
	}

	Blueprint->NewVariables.Empty();
	if (UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(Blueprint))
	{
		WidgetBlueprint->Bindings.Empty();
	}
	Blueprint->ParentClass = NewParentClass;
	FBlueprintEditorUtils::RefreshAllNodes(Blueprint);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	NormalizeWidgetVariableGuids(Cast<UWidgetBlueprint>(Blueprint));
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipSave);
	return true;
#else
	return false;
#endif
}

bool UT_InventoryMigrationLibrary::RemapBlueprintReferences(UBlueprint* Blueprint, const TArray<UBlueprint*>& SourceBlueprints, const TArray<UBlueprint*>& TargetBlueprints)
{
#if WITH_EDITOR
	if (!IsValid(Blueprint) || SourceBlueprints.Num() != TargetBlueprints.Num()) return false;

	TMap<UObject*, UObject*> ReplacementMap;
	for (int32 Index = 0; Index < SourceBlueprints.Num(); ++Index)
	{
		UBlueprint* SourceBlueprint = SourceBlueprints[Index];
		UBlueprint* TargetBlueprint = TargetBlueprints[Index];
		if (!IsValid(SourceBlueprint) || !IsValid(TargetBlueprint)) continue;

		ReplacementMap.Add(SourceBlueprint, TargetBlueprint);
		if (IsValid(SourceBlueprint->GeneratedClass) && IsValid(TargetBlueprint->GeneratedClass))
		{
			ReplacementMap.Add(SourceBlueprint->GeneratedClass, TargetBlueprint->GeneratedClass);
		}
		if (IsValid(SourceBlueprint->SkeletonGeneratedClass) && IsValid(TargetBlueprint->SkeletonGeneratedClass))
		{
			ReplacementMap.Add(SourceBlueprint->SkeletonGeneratedClass, TargetBlueprint->SkeletonGeneratedClass);
		}
	}

	FArchiveReplaceObjectRef<UObject> ReplaceBlueprintReferences(Blueprint, ReplacementMap, EArchiveReplaceObjectFlags::IgnoreOuterRef);
	TArray<UObject*> Subobjects;
	GetObjectsWithOuter(Blueprint, Subobjects, true);
	for (UObject* Subobject : Subobjects)
	{
		FArchiveReplaceObjectRef<UObject> ReplaceSubobjectReferences(Subobject, ReplacementMap, EArchiveReplaceObjectFlags::IgnoreOuterRef);
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipSave);
	return true;
#else
	return false;
#endif
}

bool UT_InventoryMigrationLibrary::RemoveWidgets(UBlueprint* Blueprint, const TArray<FName>& WidgetNames)
{
#if WITH_EDITOR
	UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(Blueprint);
	if (!IsValid(WidgetBlueprint) || !IsValid(WidgetBlueprint->WidgetTree)) return false;

	for (const FName WidgetName : WidgetNames)
	{
		WidgetBlueprint->WidgetVariableNameToGuidMap.Remove(WidgetName);
		if (UWidget* Widget = WidgetBlueprint->WidgetTree->FindWidget(WidgetName))
		{
			Widget->bIsVariable = false;
			WidgetBlueprint->WidgetTree->RemoveWidget(Widget);
		}
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
	NormalizeWidgetVariableGuids(WidgetBlueprint);
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint, EBlueprintCompileOptions::SkipSave);
	return true;
#else
	return false;
#endif
}

bool UT_InventoryMigrationLibrary::ReplaceWidgetClasses(UBlueprint* Blueprint, const TArray<UClass*>& SourceClasses, const TArray<UClass*>& TargetClasses)
{
#if WITH_EDITOR
	UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(Blueprint);
	if (!IsValid(WidgetBlueprint) || !IsValid(WidgetBlueprint->WidgetTree) || SourceClasses.Num() != TargetClasses.Num()) return false;

	TArray<UWidget*> Widgets;
	WidgetBlueprint->WidgetTree->GetAllWidgets(Widgets);
	for (UWidget* OldWidget : Widgets)
	{
		const int32 ClassIndex = SourceClasses.IndexOfByPredicate([OldWidget](const UClass* SourceClass)
		{
			return IsValid(SourceClass) && OldWidget->IsA(SourceClass);
		});
		if (ClassIndex == INDEX_NONE || !IsValid(TargetClasses[ClassIndex])) continue;

		const FName OriginalName = OldWidget->GetFName();
		UPanelWidget* Parent = OldWidget->GetParent();
		const int32 ChildIndex = Parent ? Parent->GetChildIndex(OldWidget) : INDEX_NONE;
		UPanelSlot* SlotTemplate = OldWidget->Slot ? DuplicateObject<UPanelSlot>(OldWidget->Slot, GetTransientPackage()) : nullptr;
		if (Parent && ChildIndex != INDEX_NONE) Parent->RemoveChildAt(ChildIndex);
		OldWidget->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_ForceNoResetLoaders);
		UWidget* NewWidget = WidgetBlueprint->WidgetTree->ConstructWidget<UWidget>(TargetClasses[ClassIndex], OriginalName);
		if (!IsValid(NewWidget)) return false;
		NewWidget->bIsVariable = OldWidget->bIsVariable || WidgetBlueprint->WidgetVariableNameToGuidMap.Contains(OriginalName);

		if (Parent && ChildIndex != INDEX_NONE)
		{
			Parent->InsertChildAt(ChildIndex, NewWidget, SlotTemplate);
		}
		else if (WidgetBlueprint->WidgetTree->RootWidget == OldWidget)
		{
			WidgetBlueprint->WidgetTree->RootWidget = NewWidget;
		}
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
	NormalizeWidgetVariableGuids(WidgetBlueprint);
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint, EBlueprintCompileOptions::SkipSave);
	return true;
#else
	return false;
#endif
}

bool UT_InventoryMigrationLibrary::RepairWidgetVariables(UBlueprint* Blueprint)
{
#if WITH_EDITOR
	UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(Blueprint);
	if (!IsValid(WidgetBlueprint) || !IsValid(WidgetBlueprint->WidgetTree)) return false;

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
	NormalizeWidgetVariableGuids(WidgetBlueprint);
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint, EBlueprintCompileOptions::SkipSave);
	return true;
#else
	return false;
#endif
}

bool UT_InventoryMigrationLibrary::SetWidgetTickEnabled(UBlueprint* Blueprint, bool bEnabled)
{
#if WITH_EDITOR
	UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(Blueprint);
	if (!IsValid(WidgetBlueprint)) return false;

	FProperty* TickFrequencyProperty = FindFProperty<FProperty>(UWidgetBlueprint::StaticClass(), TEXT("TickFrequency"));
	if (!TickFrequencyProperty) return false;
	void* TickFrequencyValue = TickFrequencyProperty->ContainerPtrToValuePtr<void>(WidgetBlueprint);
	if (!TickFrequencyProperty->ImportText_Direct(bEnabled ? TEXT("Auto") : TEXT("Never"), TickFrequencyValue, WidgetBlueprint, PPF_None)) return false;

	FProperty* WidgetTickFrequencyProperty = FindFProperty<FProperty>(UUserWidget::StaticClass(), TEXT("TickFrequency"));
	if (!WidgetTickFrequencyProperty) return false;
	for (UClass* WidgetClass : { WidgetBlueprint->GeneratedClass, WidgetBlueprint->SkeletonGeneratedClass })
	{
		UUserWidget* DefaultWidget = IsValid(WidgetClass) ? Cast<UUserWidget>(WidgetClass->GetDefaultObject()) : nullptr;
		if (!IsValid(DefaultWidget)) continue;
		void* DefaultTickFrequencyValue = WidgetTickFrequencyProperty->ContainerPtrToValuePtr<void>(DefaultWidget);
		if (!WidgetTickFrequencyProperty->ImportText_Direct(bEnabled ? TEXT("Auto") : TEXT("Never"), DefaultTickFrequencyValue, DefaultWidget, PPF_None)) return false;
	}
	FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBlueprint);
	return true;
#else
	return false;
#endif
}
