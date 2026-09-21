// Copyright 2025 - Roberto De Ioris

#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/LuaUnitTestState.h"
#include "Tests/LuaBlueprintPackageTest.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineBlueprintPackageTest_Simple, "LuaMachine.UnitTests.BlueprintPackage.Simple", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineBlueprintPackageTest_Simple::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	UnitTestState->RequireLuaBlueprintPackage<ULuaBlueprintPackageTest>("test");

	FLuaValue LuaTable = UnitTestState->RunString("return {x=test.test_number, y=test.test_bool, z=test.test_string, w=test.double_number(1000) }", "");

	TestTrue(TEXT("LuaTable.x == 17"), LuaTable.GetField("x").Integer == 17);
	TestTrue(TEXT("LuaTable.y == true"), LuaTable.GetField("y").Bool == true);
	TestTrue(TEXT("LuaTable.z == \"test\""), LuaTable.GetField("z").String == "test");
	TestTrue(TEXT("LuaTable.w == 2000"), LuaTable.GetField("w").Integer == 2000);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaMachineBlueprintPackageTest_StackBalance, "LuaMachine.UnitTests.BlueprintPackage.StackBalance", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLuaMachineBlueprintPackageTest_StackBalance::RunTest(const FString& Parameters)
{
	// Regression: RequireLuaBlueprintPackage popped one slot more than it pushed, so every
	// registered package left L->top one slot below the stack allocation. The next push then
	// wrote into the allocator's block header, which the heap reported only much later, from
	// lua_close in ~ULuaState (two packages were enough to make that a reliable crash).
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Inactive, false);

	ULuaUnitTestState* UnitTestState = ULuaState::CreateDynamicLuaState<ULuaUnitTestState>(TestWorld);

	TestEqual(TEXT("stack is empty before requiring"), UnitTestState->GetTop(), 0);

	UnitTestState->RequireLuaBlueprintPackage<ULuaBlueprintPackageTest>("test");
	TestEqual(TEXT("stack is balanced after the first package"), UnitTestState->GetTop(), 0);

	UnitTestState->RequireLuaBlueprintPackage<ULuaBlueprintPackageTest>("test2");
	TestEqual(TEXT("stack is balanced after the second package"), UnitTestState->GetTop(), 0);

	if (UnitTestState->GetTop() != 0)
	{
		// the VM stack is already outside its allocation: anything else run here would just corrupt the heap
		return false;
	}

	// the same lines also filled package.loaded with the wrong value (_G instead of the package table)
	FLuaValue Loaded = UnitTestState->RunString("return package.loaded.test == test and package.loaded.test2 == test2 and require('test') == test and test ~= _G", "");
	TestTrue(TEXT("package.loaded holds the package tables"), Loaded.Type == ELuaValueType::Bool && Loaded.Bool);

	return true;
}

#endif
