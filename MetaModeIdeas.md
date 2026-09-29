# Meta Mode Ideas (post-Lessons)

Ideas for extending the game after players finish the Lesson campaign. They reuse the existing systems: seeded generation, clue ranking (`ECampaignLesson`), lesson clue filtering (`InitCampaign`) and 3/2/1 scoring.

## Favored: Second Quest (merge of Mastery tiers + Hard mode progression)

This is a harder replay of the same world, like Zelda's second quest. It targets the players who get through the Lessons, who are exactly the hardest players.

### Structure
- Same tree and layout, unlocked by completing the first campaign. Different tint and a "Master" title.
- Each lesson's stages step up in size and difficulty:
  - Stage 0: 5x5 Normal
  - Stage 1: 5x5 Hard
  - Stage 2: 6x6 Normal
  - Stage 3: 6x6 Hard
  - Final: 7x7 or 8x8 Hard
- Clues are cumulative: a lesson allows every clue up to its column, like the first campaign. The generator must *require* the lesson's clue, and ideally more than once.

### Scoring changes
- Keep 3/2/1 points, but a hint on the lesson's clue gives 0 points.
- Optional per-stage **par time** as a bonus star (cosmetic, for completionists).
- The Final still completes the lesson even with hints, so nobody gets hard-stuck.

### Why it's cheap
- `InitCampaign` already takes a size, difficulty and lesson. Add a quest index to the stage table and to `UCampaignSubsystem::GetGenerationSeed`.
- `FLessonProgress` gets a second set (or a `Quest` key in the save map).
- Reuse WBP_CampaignTree and WBP_LessonPopup, with a `Quest` flag for the tint and title.
- No new assets beyond a color swap.

### Check before committing
- Whether the late clue types (Chain, AllApart, NextToEitherOr) are still *needed* on 6x6+ Hard, or whether the solver finishes with easier clues. Measure with the commandlet: the `failedToRequireLesson` rate and generation time at 5x5-8x8 Hard for every lesson.
- How long a 7x7/8x8 Hard takes to solve. If it's over ~20 minutes, cap the Final at 6x6 or 7x7.

## Other ideas

### Daily Puzzle (parked)
- One seeded puzzle per day (seed = date, e.g. `YYYYMMDD`), with a streak counter and a calendar of solved days.
- **No backend needed:** the generator is deterministic, so every device makes the same puzzle offline, and streaks and the calendar are local saves. A server is only needed for leaderboards or comparing with friends.
- Parked: not worth it unless something bigger justifies it.

### Themed puzzle packs
- Packs built around 2-4 clue types, e.g. "Spans only", "Nothing but Nots", "Either Or madness".
- 10-20 fixed seeds per pack, with the same stars and points as lessons. The generator's lesson filter does the hard part.

### Time Attack
- Fixed seeds with par times, earning stars for beating par or 2× par. Uses the existing puzzle timer.

### No-Hint / Iron mode
- No hints; wrong placements aren't allowed, or cost you. Clear a run of 5 in a row to win.

### Minimal Clues mode
- Every clue is essential, with no redundancy (skip the easy extras). A harder style of puzzle.

### Weekly Gauntlet
- One puzzle at each size from 3x3 up to 8x8 in a row; total time is the score.

### Restricted-clue puzzles
- "Solve using only horizontal clues", or "no Given, no Vertical". Generated with the lesson filter.

### Deduction score / difficulty rating
- Rate puzzles by hint-step count or by the hardest clue rank needed. Show the rating and let players chase the hardest ones.

### Puzzle creator / share codes
- The clue ID round trip lets a puzzle become a short code players share. Friend challenges could compare times.

### Story / Collection meta (rejected)
- Solved puzzles earn collectibles (icon sets, themes, board skins), and filling sets unlocks new icon themes.
- Rejected: needs more art assets than can be delivered.
