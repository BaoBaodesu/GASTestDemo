#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Inventory/T_InventoryComponent.h"
#include "Inventory/T_ItemDefinition.h"

namespace
{
	UT_InventoryComponent* MakeInventory(int32 Size)
	{
		UT_InventoryComponent* Inventory = NewObject<UT_InventoryComponent>();
		Inventory->ResizeInventory(Size);
		return Inventory;
	}

	UT_ItemDefinition* MakeItem(const TCHAR* ItemId, int32 MaxStackSize)
	{
		UT_ItemDefinition* ItemDefinition = NewObject<UT_ItemDefinition>();
		ItemDefinition->ItemId = ItemId;
		ItemDefinition->MaxStackSize = MaxStackSize;
		return ItemDefinition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTInventoryAddAndStackTest,
	"GASTestDemo1.Inventory.AddAndStack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTInventoryAddAndStackTest::RunTest(const FString& Parameters)
{
	UT_InventoryComponent* Inventory = MakeInventory(2);
	UT_ItemDefinition* ItemDefinition = MakeItem(TEXT("Test.Stack"), 10);

	int32 RemainingQuantity = 0;
	TestTrue(TEXT("物品可以加入背包"), Inventory->AddItem(ItemDefinition, 25, RemainingQuantity));
	TestEqual(TEXT("两个槽位最多接收 20 个"), RemainingQuantity, 5);
	TestEqual(TEXT("第一个槽位堆满"), Inventory->GetSlot(0).Quantity, 10);
	TestEqual(TEXT("第二个槽位堆满"), Inventory->GetSlot(1).Quantity, 10);
	TestTrue(TEXT("已满背包拒绝继续添加"), !Inventory->AddItem(ItemDefinition, 1, RemainingQuantity));
	TestEqual(TEXT("拒绝后保留原数量"), RemainingQuantity, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTInventoryMoveAndSplitTest,
	"GASTestDemo1.Inventory.MoveAndSplit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTInventoryMoveAndSplitTest::RunTest(const FString& Parameters)
{
	UT_InventoryComponent* Inventory = MakeInventory(4);
	UT_ItemDefinition* ItemA = MakeItem(TEXT("Test.A"), 10);
	UT_ItemDefinition* ItemB = MakeItem(TEXT("Test.B"), 1);

	int32 RemainingQuantity = 0;
	Inventory->AddItem(ItemA, 8, RemainingQuantity);
	TestTrue(TEXT("堆叠可拆分到指定空槽"), Inventory->SplitStack(0, 3, 2));
	TestEqual(TEXT("源槽保留数量"), Inventory->GetSlot(0).Quantity, 5);
	TestEqual(TEXT("目标槽得到拆分数量"), Inventory->GetSlot(2).Quantity, 3);
	TestTrue(TEXT("相同物品可以重新合并"), Inventory->MoveItem(2, 0));
	TestEqual(TEXT("合并后的数量正确"), Inventory->GetSlot(0).Quantity, 8);

	Inventory->AddItem(ItemB, 1, RemainingQuantity);
	const int32 ItemBIndex = Inventory->FindItemById(ItemB->ItemId);
	TestTrue(TEXT("不同物品可以交换槽位"), Inventory->MoveItem(0, ItemBIndex));
	TestEqual(TEXT("交换后物品位置正确"), Inventory->GetSlot(ItemBIndex).ItemDefinition.Get(), ItemA);
	TestTrue(TEXT("不能把全部数量拆走"), !Inventory->SplitStack(ItemBIndex, Inventory->GetSlot(ItemBIndex).Quantity));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTInventoryTransferTest,
	"GASTestDemo1.Inventory.PartialTransfer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTInventoryTransferTest::RunTest(const FString& Parameters)
{
	UT_InventoryComponent* SourceInventory = MakeInventory(2);
	UT_InventoryComponent* TargetInventory = MakeInventory(1);
	UT_ItemDefinition* ItemDefinition = MakeItem(TEXT("Test.Transfer"), 10);

	int32 RemainingQuantity = 0;
	SourceInventory->AddItem(ItemDefinition, 5, RemainingQuantity);
	TargetInventory->AddItem(ItemDefinition, 8, RemainingQuantity);

	TestTrue(TEXT("目标空间不足时允许部分转移"), SourceInventory->TransferItem(TargetInventory, 0, 5, RemainingQuantity));
	TestEqual(TEXT("返回未转移数量"), RemainingQuantity, 3);
	TestEqual(TEXT("源背包只扣成功转移数量"), SourceInventory->GetSlot(0).Quantity, 3);
	TestEqual(TEXT("目标堆叠达到上限"), TargetInventory->GetSlot(0).Quantity, 10);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTInventoryValidationAndRollbackTest,
	"GASTestDemo1.Inventory.ValidationAndRollback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTInventoryValidationAndRollbackTest::RunTest(const FString& Parameters)
{
	UT_InventoryComponent* Inventory = MakeInventory(2);
	UT_ItemDefinition* ItemDefinition = MakeItem(TEXT("Test.Validation"), 1);

	int32 RemainingQuantity = 0;
	Inventory->AddItem(ItemDefinition, 2, RemainingQuantity);
	TestTrue(TEXT("可以按物品 ID 查询总数量"), Inventory->ContainsItem(ItemDefinition->ItemId, 2));
	TestEqual(TEXT("物品总数量正确"), Inventory->GetTotalQuantity(ItemDefinition->ItemId), 2);
	TestTrue(TEXT("无效槽位移动被拒绝"), !Inventory->MoveItem(INDEX_NONE, 0));
	TestTrue(TEXT("零数量移动被拒绝"), !Inventory->MoveItem(0, 1, 0));
	TestTrue(TEXT("超过源堆数量的移动被拒绝"), !Inventory->MoveItem(0, 1, 2));

	TestTrue(TEXT("没有世界和拾取类时丢弃失败"), !Inventory->DropItem(0, 1));
	TestEqual(TEXT("丢弃生成失败不扣除物品"), Inventory->GetSlot(0).Quantity, 1);
	TestTrue(TEXT("裁剪槽位无法生成掉落物时取消缩容"), !Inventory->ResizeInventory(1));
	TestEqual(TEXT("缩容失败后槽位数量不变"), Inventory->GetInventorySize(), 2);
	TestEqual(TEXT("缩容失败后物品总量不变"), Inventory->GetTotalQuantity(ItemDefinition->ItemId), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTInventoryQuickSlotTest,
	"GASTestDemo1.Inventory.QuickSlot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTInventoryQuickSlotTest::RunTest(const FString& Parameters)
{
	UT_InventoryComponent* Inventory = MakeInventory(2);
	UT_ItemDefinition* ItemDefinition = MakeItem(TEXT("Test.QuickSlot"), 2);

	int32 RemainingQuantity = 0;
	Inventory->AddItem(ItemDefinition, 1, RemainingQuantity);
	TestTrue(TEXT("Assign item to quick slot"), Inventory->AssignQuickSlot(0, ItemDefinition));
	TestEqual(TEXT("Quick slot stores item definition"), Inventory->GetQuickSlotItem(0), ItemDefinition);

	Inventory->MoveItem(0, 1);
	TestEqual(TEXT("Moving item keeps quick slot assignment"), Inventory->GetQuickSlotItem(0), ItemDefinition);

	Inventory->RemoveItem(1, 1);
	TestNull(TEXT("Removing final item clears quick slot"), Inventory->GetQuickSlotItem(0));
	return true;
}

#endif
