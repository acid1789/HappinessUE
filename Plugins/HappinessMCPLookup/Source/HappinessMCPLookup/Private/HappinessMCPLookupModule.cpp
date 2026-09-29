#include "Modules/ModuleManager.h"
#include "Misc/CoreDelegates.h"
#include "Engine/Engine.h"
#include "ToolsetRegistry/UToolsetRegistry.h"
#include "HappinessMCPLookupToolset.h"
#include "HappinessUIToolset.h"

class FHappinessMCPLookupModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		// The module loads early so its commandlet can be found; the toolset registry is only
		// ready once the engine has initialized
		if (GEngine && GEngine->IsInitialized())
		{
			Register();
		}
		else
		{
			PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddRaw(this, &FHappinessMCPLookupModule::Register);
		}
	}

	virtual void ShutdownModule() override
	{
		FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitHandle);

		if (bRegistered && UObjectInitialized())
		{
			UToolsetRegistry::UnregisterToolsetClass(UHappinessMCPLookupToolset::StaticClass());
			UToolsetRegistry::UnregisterToolsetClass(UHappinessUIToolset::StaticClass());
		}
	}

private:
	void Register()
	{
		UToolsetRegistry::RegisterToolsetClass(UHappinessMCPLookupToolset::StaticClass());
		UToolsetRegistry::RegisterToolsetClass(UHappinessUIToolset::StaticClass());
		bRegistered = true;
	}

	FDelegateHandle PostEngineInitHandle;
	bool bRegistered = false;
};

IMPLEMENT_MODULE(FHappinessMCPLookupModule, HappinessMCPLookup)
