# Tutorial Steps

The tutorial puzzle's steps by number (`TutorialStep` in `Source/Happiness/UI/GamePanelWidget.cpp`). Refer to a step by its number.

```
Puzzle: seed 129, 3x3. Rows and columns counted from 1.

         Column 1    Column 2    Column 3
Row 1    Heart       Triangle    Star
Row 2    Phoenix     Dragon      Unicorn (given)
Row 3    Butterfly   Bee         Ladybug
(the solution)

Touch only: on touch, setting an icon goes through the cell window
(select it, Confirm, Commit). With a mouse the player clicks the icon in
the cell instead, Next appears at once, and those steps are skipped.
```

```
┌───────┬────────────────────────────┬──────────────────────────────────────────────────────┬──────────────────────────┐
│ #     │ Focus                      │ Text                                                 │ Moves on                 │
├───────┴────────────────────────────┴──────────────────────────────────────────────────────┴──────────────────────────┤
│ INTRODUCTION                                                                                                         │
├───────┬────────────────────────────┬──────────────────────────────────────────────────────┬──────────────────────────┤
│ 0     │ The whole grid (closes in  │ This is the puzzle grid.                             │ Next                     │
│       │ from the screen edges)     │ The objective of the game is to figure out where     │                          │
│       │                            │ each icon belongs in the grid.                       │                          │
├───────┼────────────────────────────┼──────────────────────────────────────────────────────┼──────────────────────────┤
│ 1     │ The grid. Rows get colors: │ The puzzle has one category per row. Each icon in a  │ Next                     │
│       │ row 1, then 0.5 s later    │ category belongs in one of the columns.              │                          │
│       │ row 2, then row 3          │                                                      │                          │
├───────┼────────────────────────────┼──────────────────────────────────────────────────────┼──────────────────────────┤
│ 2     │ The grid. Columns get the  │ Your goal is to figure out which column each icon    │ Next                     │
│       │ colors instead (1, 2, 3);  │ belongs in.                                          │                          │
│       │ they stay for the rest of  │                                                      │                          │
│       │ the tutorial               │                                                      │                          │
├───────┼────────────────────────────┼──────────────────────────────────────────────────────┼──────────────────────────┤
│ 3     │ The vertical clues         │ These are vertical clues.                            │ Next                     │
│       │                            │ They describe true things about a single column.     │                          │
│       │                            │ You can use this information to determine where      │                          │
│       │                            │ icons belong in a single column of the puzzle grid.  │                          │
├───────┼────────────────────────────┼──────────────────────────────────────────────────────┼──────────────────────────┤
│ 4     │ The horizontal clues       │ These are horizontal clues.                          │ Next                     │
│       │                            │ They describe true things about multiple columns.    │                          │
│       │                            │ They compare columns left to right, no matter which  │                          │
│       │                            │ row each icon is in.                                 │                          │
├───────┴────────────────────────────┴──────────────────────────────────────────────────────┴──────────────────────────┤
│ STAR (SAME COLUMN)                                                                                                   │
├───────┬────────────────────────────┬──────────────────────────────────────────────────────┬──────────────────────────┤
│ 5     │ 1st vertical clue (Star /  │ This clue says [star] and [unicorn] are in the same  │ Next                     │
│       │ Unicorn)                   │ column.                                              │                          │
├───────┼────────────────────────────┼──────────────────────────────────────────────────────┼──────────────────────────┤
│ 6     │ 1st vertical clue + column │ Since we know where the [unicorn] is, the [star]     │ Touch: tap the cell      │
│       │ 3, rows 1-2                │ must be in this column too.                          │ Mouse: click the star,   │
│       │                            │ Touch: Tap the cell with the [star] above the        │ then Next                │
│       │                            │ [unicorn].                                           │                          │
│       │                            │ Mouse: Left Click the [star] icon to set it in this  │                          │
│       │                            │ spot in the grid.                                    │                          │
├───────┼────────────────────────────┼──────────────────────────────────────────────────────┼──────────────────────────┤
│ 7     │ (touch only) The star in   │ Tap on the [star] icon to select it.                 │ Select it                │
│       │ the cell window            │                                                      │                          │
├───────┼────────────────────────────┼──────────────────────────────────────────────────────┼──────────────────────────┤
│ 8     │ (touch only) Confirm       │ Tap the Confirm button to set the [star] in this     │ Confirm                  │
│       │                            │ cell.                                                │                          │
├───────┼────────────────────────────┼──────────────────────────────────────────────────────┼──────────────────────────┤
│ 9     │ (touch only) Commit        │ Tap the Commit button to apply this change to the    │ Commit                   │
│       │                            │ puzzle.                                              │                          │
├───────┴────────────────────────────┴──────────────────────────────────────────────────────┴──────────────────────────┤
│ TRIANGLE (NEXT TO)                                                                                                   │
├───────┬────────────────────────────┬──────────────────────────────────────────────────────┬──────────────────────────┤
│ 10    │ Horizontal clue (Span)     │ This clue says the [triangle] is in a column next to │ Next                     │
│       │                            │ the [star].                                          │                          │
├───────┼────────────────────────────┼──────────────────────────────────────────────────────┼──────────────────────────┤
│ 11    │ Horizontal clue + row 1,   │ There is only one place that [triangle] can go to    │ Touch: tap the cell      │
│       │ columns 2-3                │ comply with the clue.                                │ Mouse: click, then Next  │
│       │                            │ Touch: Tap the middle cell containing the            │                          │
│       │                            │ [triangle].                                          │                          │
│       │                            │ Mouse: Left click the [triangle] in the middle cell  │                          │
│       │                            │ to set it here.                                      │                          │
├───────┼────────────────────────────┼──────────────────────────────────────────────────────┼──────────────────────────┤
│ 12-14 │ (touch only) Select,       │ As steps 7-9                                         │ As steps 7-9             │
│       │ Confirm, Commit for the    │                                                      │                          │
│       │ triangle                   │                                                      │                          │
├───────┼────────────────────────────┼──────────────────────────────────────────────────────┼──────────────────────────┤
│ 15    │ The heart's cell (row 1,   │ Notice the [heart] is now set here as the only       │ Next                     │
│       │ column 1)                  │ option left for this cell.                           │                          │
├───────┴────────────────────────────┴──────────────────────────────────────────────────────┴──────────────────────────┤
│ PHOENIX (SPAN)                                                                                                       │
├───────┬────────────────────────────┬──────────────────────────────────────────────────────┬──────────────────────────┤
│ 16    │ Horizontal clue            │ This clue also says the [phoenix] is in a column     │ Next                     │
│       │                            │ next to the [triangle]                               │                          │
├───────┼────────────────────────────┼──────────────────────────────────────────────────────┼──────────────────────────┤
│ 17    │ Horizontal clue + rows     │ The [star] is in the right column. The [triangle] is │ Touch: tap the cell      │
│       │ 1-2, columns 1-2           │ in the middle column. The clue says the next column  │ Mouse: click, then Next  │
│       │                            │ should have the [phoenix], so the [phoenix] belongs  │                          │
│       │                            │ on the left column.                                  │                          │
│       │                            │ Touch: Tap the far left cell containing the          │                          │
│       │                            │ [phoenix]                                            │                          │
│       │                            │ Mouse: Left click the [phoenix] to set it here.      │                          │
├───────┼────────────────────────────┼──────────────────────────────────────────────────────┼──────────────────────────┤
│ 18-20 │ (touch only) Select,       │ As steps 7-9                                         │ As steps 7-9             │
│       │ Confirm, Commit for the    │                                                      │                          │
│       │ phoenix                    │                                                      │                          │
├───────┴────────────────────────────┴──────────────────────────────────────────────────────┴──────────────────────────┤
│ HINT AND BEE                                                                                                         │
├───────┬────────────────────────────┬──────────────────────────────────────────────────────┬──────────────────────────┤
│ 21    │ The Hint button (and its   │ If you don't know what to do next, press the Hint    │ The hint's Confirm       │
│       │ confirm window)            │ button. Hints wont hurt you, they can only help you  │ (Cancel stays here)      │
│       │                            │ to improve. Press the Hint button now.               │                          │
├───────┼────────────────────────────┼──────────────────────────────────────────────────────┼──────────────────────────┤
│ 22    │ 2nd vertical clue (Dragon  │ This clue can help you right now. The highlighted    │ Next                     │
│       │ / Bee) + column 2, rows    │ icon is the one it's talking about, and the          │                          │
│       │ 2-3. Text at the top       │ description tells you what to do with it.            │                          │
├───────┼────────────────────────────┼──────────────────────────────────────────────────────┼──────────────────────────┤
│ 23    │ Same circles               │ Follow the hint description and set the [bee] in the │ Touch: tap the cell      │
│       │                            │ middle column with the [dragon]                      │ Mouse: click, then Next  │
├───────┼────────────────────────────┼──────────────────────────────────────────────────────┼──────────────────────────┤
│ 24-26 │ (touch only) Select,       │ As steps 7-9                                         │ As steps 7-9             │
│       │ Confirm, Commit for the    │                                                      │                          │
│       │ bee                        │                                                      │                          │
├───────┴────────────────────────────┴──────────────────────────────────────────────────────┴──────────────────────────┤
│ LADYBUG (NOT SAME COLUMN, ELIMINATION)                                                                               │
├───────┬────────────────────────────┬──────────────────────────────────────────────────────┬──────────────────────────┤
│ 27    │ 3rd vertical clue (Phoenix │ This clue says the [ladybug] is not in the same      │ Touch: tap the cell      │
│       │ / not Ladybug) + column 1, │ column as the [phoenix].                             │ Mouse: right click, then │
│       │ rows 2-3. Text below the   │ Touch: Tap the cell below the [phoenix] to eliminate │ Next                     │
│       │ grid                       │ the [ladybug] from it.                               │                          │
│       │                            │ Mouse: Right click the [ladybug] below the [phoenix] │                          │
│       │                            │ to eliminate it from this cell.                      │                          │
├───────┼────────────────────────────┼──────────────────────────────────────────────────────┼──────────────────────────┤
│ 28    │ (touch only) The ladybug   │ Tap on the [ladybug] icon to select it.              │ Select it                │
│       │ in the cell window         │                                                      │                          │
├───────┼────────────────────────────┼──────────────────────────────────────────────────────┼──────────────────────────┤
│ 29    │ (touch only) Eliminate     │ Tap the Eliminate button to remove the [ladybug]     │ Eliminate                │
│       │                            │ from this cell.                                      │                          │
├───────┼────────────────────────────┼──────────────────────────────────────────────────────┼──────────────────────────┤
│ 30    │ (touch only) Commit        │ Tap the Commit button to apply this change to the    │ Commit                   │
│       │                            │ puzzle.                                              │ (the game fills in the   │
│       │                            │                                                      │ rest: solved, no         │
│       │                            │                                                      │ fireworks or end screen) │
├───────┴────────────────────────────┴──────────────────────────────────────────────────────┴──────────────────────────┤
│ FINISH                                                                                                               │
├───────┬────────────────────────────┬──────────────────────────────────────────────────────┬──────────────────────────┤
│ 31    │ The whole grid. Text at    │ This is what a fully solved puzzle looks like. All   │ Restart Tutorial (plays  │
│       │ the bottom of the screen   │ icons are set correctly in their places. Now you are │ it again)                │
│       │                            │ ready to move on to the Lessons and learn how each   │ Learn More (Game Rules,  │
│       │                            │ clue works. If you want more information, check out  │ then back)               │
│       │                            │ the Game Rules.                                      │ Proceed to Lessons (Game │
│       │                            │                                                      │ Basics)                  │
│       │                            │                                                      │ Reaching this step marks │
│       │                            │                                                      │ the tutorial completed   │
└───────┴────────────────────────────┴──────────────────────────────────────────────────────┴──────────────────────────┘
```
