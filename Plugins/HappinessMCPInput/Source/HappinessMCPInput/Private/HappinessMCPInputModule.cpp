#include "Modules/ModuleManager.h"
#include "ToolsetRegistry/UToolsetRegistry.h"
#include "HappinessMCPInputToolset.h"

class FHappinessMCPInputModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UToolsetRegistry::RegisterToolsetClass(UHappinessMCPInputToolset::StaticClass());
	}

	virtual void ShutdownModule() override
	{
		if (UObjectInitialized())
		{
			UToolsetRegistry::UnregisterToolsetClass(UHappinessMCPInputToolset::StaticClass());
		}
	}
};

IMPLEMENT_MODULE(FHappinessMCPInputModule, HappinessMCPInput)
