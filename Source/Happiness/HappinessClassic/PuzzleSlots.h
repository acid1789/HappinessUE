#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CampaignLesson.h"
#include "PuzzleSlots.generated.h"

class UHappinessSaveGame;

/** The modes that each keep a puzzle in progress, in their own save slot */
UENUM(BlueprintType)
enum class EPuzzleSlot : uint8
{
	None,
	Daily,
	FreePlay,
	Lessons,
	Campaign,
};

/** What a mode's puzzle slot records besides the puzzle, to tell whether it can still be resumed */
USTRUCT()
struct FPuzzleSlotInfo
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	EPuzzleSlot Slot = EPuzzleSlot::None;

	/** Lessons and Campaign: the lesson session the puzzle belongs to, and the stage's seed when it was saved */
	UPROPERTY(SaveGame)
	ECampaignLesson Lesson = ECampaignLesson::Given;

	UPROPERTY(SaveGame)
	int32 Stage = 0;

	UPROPERTY(SaveGame)
	int32 Seed = 0;

	/** Daily: the puzzle's date (UDailySubsystem::GetToday) */
	UPROPERTY(SaveGame)
	int32 Date = 0;
};

/**
 * One puzzle in progress per mode (Daily, Free Play, Lessons, Campaign), each in its own save slot
 * (HappinessPuzzle_<Mode>), so leaving a puzzle for another mode keeps it.
 *
 * PC_Happiness saves its puzzle into the game save (SG_Happiness) on every move. As that save is written,
 * UHappinessSaveGame hands it here and its puzzle (every SG_Happiness variable but the player's level and
 * experience) is copied into the slot of the mode the puzzle was started in (NotePuzzleStarted, from
 * UCampaignSubsystem::InitPuzzleForPlay).
 *
 * The game always starts on the mode screen. Choosing a mode resumes its slot's puzzle if there is one that can still
 * be played (CanResume): BeginResume, then PC_Happiness's LoadHappiness (through WBP_GameSelect's
 * ShowHappinessConfig) loads the game save, into which UHappinessSaveGame copies that slot's puzzle (or none), then
 * EndResume. Free Play always goes that way: with no puzzle, the screen opens the Free Play settings.
 */
UCLASS()
class HAPPINESS_API UPuzzleSlotSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** A puzzle has just been set up for play: its saves go to the slot of the mode it belongs to */
	static void NotePuzzleStarted(const UObject* WorldContextObject);

	/** Slot's saved puzzle can be resumed (Daily: today's, unsolved; Lessons and Campaign: its stage still open). Info: its record. */
	bool CanResume(EPuzzleSlot Slot, FPuzzleSlotInfo* OutInfo = nullptr) const;

	/** The player gave up Slot's puzzle: it's no longer in progress (its slot keeps the rest, e.g. Free Play's last puzzle number) */
	void AbandonPuzzle(EPuzzleSlot Slot);

	/** The next load of the game save gets Slot's puzzle, or none if it has none, until EndResume */
	void BeginResume(EPuzzleSlot Slot) { ResumeSlot = Slot; }
	void EndResume() { ResumeSlot = EPuzzleSlot::None; }

	/** The mode of the puzzle being played (None before the first one) */
	EPuzzleSlot GetPlayingSlot() const { return PlayingSlot; }

	/** UHappinessSaveGame: a game save is being written; copy its puzzle into the playing mode's slot */
	void WriteSlot(const UHappinessSaveGame& GameSave);

	/** UHappinessSaveGame: a game save has been loaded; between BeginResume and EndResume, put the slot's puzzle in it */
	void ApplyResume(UHappinessSaveGame& GameSave);

	/** Delete every mode's puzzle slot (UHappinessSaveGame::ResetOldSave) */
	static void DeleteAllSlots();

	/** A slot is being read or written: its save game mustn't be handed back here */
	static bool IsSlotIO() { return bSlotIO; }

	static UPuzzleSlotSubsystem* Get(const UObject* WorldContextObject);
	static UPuzzleSlotSubsystem* GetInstance() { return Instance.Get(); }

private:
	static FString GetSlotName(EPuzzleSlot Slot);
	UHappinessSaveGame* LoadSlot(EPuzzleSlot Slot) const;

	/** A game save from before slots: its puzzle goes to the slot of the mode it was being played in */
	void MigrateOldSave();

	EPuzzleSlot PlayingSlot = EPuzzleSlot::None;
	EPuzzleSlot ResumeSlot = EPuzzleSlot::None;

	static bool bSlotIO;
	static TWeakObjectPtr<UPuzzleSlotSubsystem> Instance;
};
