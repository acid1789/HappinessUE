# Happiness — Art Direction & Codex Handoff

## Purpose

This document defines the visual direction for **Happiness** and provides context for Codex when acting as the project's **art director and UI/visual-design assistant**.

Codex is not expected to take ownership of gameplay programming or general game logic. Claude is currently being used for those responsibilities.

Codex should focus primarily on:

- Art direction
- UI visual design
- Screen composition
- Asset planning
- Image generation and iteration
- Maintaining visual consistency
- Evaluating artwork in the context of the actual game
- Integrating or helping integrate approved visual assets when appropriate
- Using its access to the project to understand how artwork will actually appear in-game

Other project documentation describes how to build, run, inspect, interact with, and modify the Unreal project. Use those documents rather than duplicating those instructions here.

---

# 1. Game Overview

**Happiness** is a 2D icon-based logic puzzle game built in Unreal Engine.

Primary target:

- Mobile
- Landscape orientation only

A PC release may also be considered later.

The visual design should therefore work well across common landscape mobile aspect ratios while avoiding unnecessary assumptions that would make eventual PC/wider-screen support difficult.

The game is intended for a **broad audience**.

It is not intended to visually target a specific genre, demographic, fandom, or niche.

The desired impression is:

- Welcoming
- Polished
- Relaxing
- Colorful
- Friendly
- High quality
- Easy to understand
- Appropriate for a casual puzzle game

Avoid making the game feel specifically like:

- A fantasy game
- An RPG
- A children's game
- A sci-fi game
- A casino game
- A match-3 clone
- A generic minimalist mobile app

The goal is broad appeal without becoming visually anonymous.

---

# 2. Existing Puzzle Icons

The project contains an existing collection of puzzle icons.

These icons are important **style references**, but they are NOT the theme of the game.

Do not design the rest of the game around the specific objects currently represented by the icons.

The icon library can change and grow substantially over time. Future categories might contain completely different subject matter.

Therefore:

> Treat the existing icons as examples of the game's illustration/rendering language, not as a catalog of imagery that should appear throughout the UI.

For example, do NOT automatically decorate menus with existing food, flower, sports, musical instrument, or hat icons simply because those categories currently exist.

The title screen in particular should not display the puzzle icons merely to communicate that the game contains icons.

---

# 3. Visual Characteristics to Preserve From the Icons

The existing icons establish several useful characteristics for the broader visual language.

These include:

- Stylized 2D illustration
- Clean, immediately recognizable silhouettes
- Strong visual readability
- Dark graphic outlines where appropriate
- Rich but controlled color
- Soft dimensional shading
- Controlled highlights
- Slightly exaggerated forms when they improve readability
- Subtle painted/illustrated texture
- Tangible forms with some sense of depth
- Good readability at mobile scale
- A polished casual-game finish

Artwork should feel illustrated rather than photorealistic.

It should have dimensionality without looking like raw 3D renders.

It should feel friendly without relying on excessive cuteness.

### Avoid

- Photorealism
- Flat minimalist vector/app-icon styling
- Heavy realism
- Excessively thick cartoon outlines
- Chibi/cute-for-the-sake-of-cute styling
- Excessive gloss
- Plastic-looking 3D rendering
- Highly ornate fantasy rendering
- Sci-fi styling
- Excessive visual noise

---

# 4. Broader Art Style

A useful general description of the desired style is:

> Polished illustrated casual-puzzle game art with clean silhouettes, rich colors, subtle graphic outlines, soft dimensional shading, restrained highlights, gentle painted texture, and a warm handcrafted quality. Artwork should be cheerful and approachable without feeling childish or excessively cute. It should remain clear and readable at mobile scale.

This description is intentionally broader than the current icon set.

The visual system must be capable of supporting future artwork such as:

- Objects
- Animals
- Food
- Plants
- Vehicles
- Buildings
- Clothing
- Tools
- People
- Landscapes
- Abstract concepts
- UI decoration

without requiring a fundamental change in style.

---

# 5. Background Direction

Background artwork should generally be created as **reusable artwork**, separate from interactive UI.

Do not bake the following into general-purpose backgrounds unless explicitly requested:

- Buttons
- Labels
- Menu text
- Game title
- Version information
- Interactive controls

These elements should normally remain separate UMG/UI elements.

This allows a background to be reused across:

- Title screen
- Main menu
- Settings
- Puzzle selection
- Other menu screens

where appropriate.

---

# 6. Current Background Exploration

Initial visual exploration produced a promising direction based around a bright, welcoming illustrated landscape.

The current concept uses imagery such as:

- Open blue sky
- Soft clouds
- Distant mountains
- Calm blue water
- Warm sunlight
- Green vegetation
- Stone architecture/terrace elements
- Small amounts of colorful flowers
- Painterly stylization
- Gentle depth

The first concept was promising but too vertically composed and somewhat busy.

The subsequent direction moved toward:

- Landscape composition
- Significantly more open space
- Fewer foreground objects
- Lower visual density
- Large quiet areas suitable for UI
- Decorative/environmental detail concentrated primarily around edges
- A calm central vista

This is currently a **promising direction, not a locked final design**.

Do not assume Mediterranean scenery, lakes, mountains, flowers, stone terraces, or any other specific element is mandatory.

Those elements were successful primarily because they created an overall feeling that was:

- Pleasant
- Bright
- Welcoming
- Relaxed
- Colorful
- Non-threatening
- Broadly appealing

Preserve those qualities while remaining willing to explore other imagery.

---

# 7. Composition Principles for UI Backgrounds

Because UI will be placed over these backgrounds, composition is extremely important.

Prefer:

- Quiet central areas
- Large low-detail regions
- Clear value separation behind UI
- Decorative detail toward edges and corners
- Natural framing
- Depth without clutter
- Strong overall composition when viewed at mobile size

Avoid placing important high-frequency detail directly behind likely UI areas.

The background should support the interface rather than compete with it.

Do not fill empty space merely because space is available.

**Negative space is useful.**

---

# 8. Landscape Orientation

Happiness is locked to landscape orientation on mobile.

Backgrounds and major screen compositions should therefore be designed natively for landscape rather than generated as portrait artwork and cropped afterward.

When designing important backgrounds, consider how the composition behaves across multiple landscape aspect ratios.

Important focal content should not be placed so close to the extreme edges that normal cropping or safe-area adjustments destroy the composition.

When possible, think in terms of:

- Safe central composition
- Expandable peripheral scenery
- UI-safe regions
- Reasonable cropping tolerance

This will also help if a PC version is produced later.

---

# 9. UI Art Direction

The exact UI visual system has NOT yet been finalized.

Do not infer that the puzzle icon frames must become the universal UI frame style.

In particular, the current icons contain dark tile backgrounds and light beveled/octagonal borders.

Those are characteristics of the **puzzle icons**, not necessarily characteristics of:

- Menu panels
- Buttons
- Dialogs
- Title treatment
- Navigation
- Settings screens

The eventual UI should visually coexist with the icons without simply copying their frames.

A successful UI should feel as though the UI and icons were created by the same overall art team while still serving different functions.

---

# 10. Title / Logo

The user selected **option F** from `output/imagegen/happiness-logo-background-directions-v2.png`:
warm ivory, elegant italic serif lettering with gently sweeping forms and a restrained painted finish.
This is the approved title direction; the integrated asset remains subject to in-game review.

The transparent production version is `output/imagegen/happiness-logo-F-final.png`, imported as
`/Game/General/Textures/T_HappinessLogo_F`. `WBP_GameSelect` uses it in every state of the existing
`HappinessClassic_Btn`, replacing the legacy smiley logo while retaining its click behavior. Keep the
lettering separate from background artwork. The local source PNGs and generation prompts are ignored by Git.

The reference background supplied by the user is
`output/imagegen/Mediterranean Terrace Coastal Vista.png`. It is imported as
`/Game/General/Textures/T_VillaBackground` and assigned to the full-screen `Background` image in
`WBP_GameSelect`, replacing `Clouds4`. The F title remains a separate transparent UI asset.

Treat this as a separate design problem from the background.

Do not permanently bake the word "Happiness" into reusable background artwork.

The logo should eventually:

- Be readable instantly
- Work at mobile resolution
- Match the overall illustrated visual language
- Feel friendly and polished
- Avoid implying a narrow genre
- Work over multiple backgrounds where practical

Explore the logo separately when appropriate.

---

# 11. Image Generation Guidelines

When generating new artwork, use the existing approved assets as visual references whenever doing so improves consistency.

A useful general prompt foundation is:

> Create polished 2D game artwork in a warm illustrated casual-game style. Use clean readable forms, rich controlled colors, subtle graphic outlines, soft dimensional shading, restrained highlights, gentle painted texture, and a premium casual-game finish. Maintain excellent readability at mobile scale. The result should feel welcoming and sophisticated without being childish, photorealistic, minimalist, fantasy-specific, or sci-fi.

This is a starting point, not wording that must be copied literally into every prompt.

Prompts should be adapted to the particular asset.

---

# 12. Asset Generation Principles

Clue overlays encode puzzle rules. Preserve their functional silhouette when restyling them.
The **span** overlay must retain a straight double-headed arrow with clear outward arrow points at
both ends: those points tell the player the clue is bidirectional. A grouping bracket is not a valid
replacement. Inspect the original symbol at game size before generating a replacement.
The **not** overlay must keep its circular ring and upper-left-to-lower-right diagonal slash, with an
open transparent interior so the excluded icon remains identifiable. `T_NotBackedPainted` includes the
diagonal in its artwork; `Overlay_Not` in both clue widgets therefore uses a zero-degree render
rotation. The legacy texture needed 45 degrees. Check existing widget transforms when replacing art.
For **directly left of**, the user selected option C: a short terracotta connector at the seam between
the two icons, with a vertical terminal on the left and one right-pointing arrowhead. It conveys
adjacency in a fixed left-to-right order. The production texture is `T_DirectlyLeftOfBackedPainted`, used by
`Overlay_DirectlyLeftOf` in `WBP_HorizontalClue`; the source is `output/imagegen/directly-left-of-backed-v1.png`.

The **between** overlay retains horizontal ellipses over the shared icon borders: three terracotta dots
on a compact dark umber capsule with a sandstone rim. `T_BetweenDotsPainted` is used by `Overlay_DotsL/R`.
Its portrait source includes transparent padding to preserve the existing 30-by-60 slots; these slots
also serve the all-apart divider, whose runtime texture override must remain intact.
The **chain** overlay retains its two staggered right-pointing arrows and existing slot positions.
`T_ChainArrowBackedPainted` matches the terracotta, sandstone highlights and umber edges of the span arrow.
Its transparent lower padding keeps the arrow in the upper part of each existing slot.
Sources and generation prompts are `output/imagegen/between-painted-v1.*` and `chain-backed-v1.*`.
The **either/or** overlay retains two opposing curved arrows with an open transparent center.
`T_EitherOrBackedPainted` replaces `EitherOrOverlay` in both clue widgets: the vertical clue keeps zero
rotation, and horizontal `Overlay_EORL/R` keep 90 degrees. Preserve the existing positions and
directions; this symbol represents alternative choices, not a closed circular action button.
Each arrow has its own curved dark umber backing with a sandstone rim, matching the between badges;
the open center remains transparent so the icons stay readable. The initial unbacked version was
too difficult to distinguish from the icon artwork.
Source and generation prompt: `output/imagegen/either-or-backed-v2.*`.

The user approved the either/or backing and requested it for all previously restyled overlays.
Span (`T_SpanArrowBackedPainted`), not, directly-left-of and chain therefore use dark umber backing
that follows their functional silhouettes, edged with a thin sandstone rim. Keep the icon openings
transparent and retain the existing widget slots and transforms. Sources and built-in generation
prompts are `output/imagegen/span-backed-v1.*`, `not-backed-v1.*`, `directly-left-of-backed-v1.*`
and `chain-backed-v1.*`. The earlier unbacked artwork remains available for reference.

The **edge / not-edge** overlays retain five square boxes on one continuous dark umber badge with
a sandstone rim. Edge fills only the two end boxes; not-edge fills the three middle boxes and leaves
both ends hollow. These represent end columns versus interior columns, not a fixed five-column board.
`Overlay_Edge` uses `T_EdgeBoxesPainted`; the NotEdge branch in `Populate` switches its brush to
`T_NotEdgeBoxesPainted`. Preserve that runtime switch as well as the existing slot placement.
Sources and built-in generation prompts: `output/imagegen/edge-boxes-painted-v1.*` and
`not-edge-boxes-painted-v1.*`.

The **gap** overlay is bidirectional: two clear outward arrowheads in front of one exaggerated hollow,
dashed tile centered vertically in the clue. Its top edge sits behind the arrow. The empty tile means
exactly one intervening column (`abs(C0-C1) == 2`), and distinguishes gap from span.
`T_GapPaintedCentered` uses the same terracotta, umber backing and sandstone
rim as the other overlays. The Gap branch in `WBP_HorizontalClue.Populate` assigns it to `Overlay_Span`;
the widget's default brush remains `T_SpanArrowBackedPainted` for span clues. Preserve its 180-by-60
slot and the transparent areas over the endpoint icons. Source and built-in generation prompt:
`output/imagegen/gap-painted-v2.*`.

All Apart uses `T_AllApartDividerPainted`: two straight terracotta vertical
separators on narrow dark umber badges with sandstone rims. They keep the existing
30x60 seam positions between the three icons, with no directional arrows.
`Populate` switches both shared `Overlay_DotsL/R` textures for All Apart only;
their default Between ellipsis brushes remain unchanged. Source and prompt:
`output/imagegen/all-apart-painted-v1.png` and its `.prompt.txt` companion.

The user selected the sage clue frame combined with corner accents in the same
color family. `T_ClueSelectionSage` replaces the red selection markers in both
`WBP_HorizontalClue` and `WBP_VerticalClue`. A transparent nine-slice Box brush
keeps the thicker sage corners intact in either orientation; the icons stay
uncovered. Selection remains hidden until the existing Blueprint selection logic
shows it. Source and prompt: `output/imagegen/clue-selection-sage-v1.*`.

Grid cells now use `T_VillaCellFrameThin`, a sandstone nine-slice frame with
straight outer edges flush to the texture bounds. Its 1254px source has an
approximately 39px opaque side stroke; the 78px UI texture gives about 2.4 layout
pixels of visible border. Rounded corner cutouts remain transparent. This removes
the old transparent gutter between adjoining row frames. Source and prompt:
`output/imagegen/villa-cell-frame-thin-v1.*`. The game panel's outer frame uses
the same flush texture so the outer cells do not protrude into the old frame's
transparent gutter. Clue frames retain their existing textures.

`WBP_Cell` inherits `UGameCellWidget`, which wraps candidate and resolved icon
containers in a runtime Retainer Box. `M_CellRoundedMask` uses a rounded rectangle
with a 6-layout-pixel radius and the cell's current dimensions, preserving square
icons while clipping the corners. The sandstone frame draws above the masked
content. Retainers redraw on invalidation and size changes, rather than on every
frame. Test with a live PIE capture: the standalone offscreen widget renderer does
not reliably draw dynamically added retained content.

The inline clue description (`WBP_HelpPanel`, called the Hint Panel in the UI)
centers its text and icons over the puzzle. `UGamePanelWidget` collapses it for
7x7 and 8x8 boards and moves the board top to the description strip's top,
reclaiming 35 layout pixels with the current screen offsets. Switching to a
smaller puzzle restores the original board top and description visibility.

For isolated assets:

- Prefer transparent backgrounds.
- Generate a single isolated asset unless a set is explicitly requested.
- Leave useful padding.
- Avoid unnecessary scenery or presentation mockups.
- Avoid text unless specifically required.
- Maintain consistent lighting and rendering.

For backgrounds:

- Do not use transparency unless there is a specific reason.
- Compose for the actual target aspect ratio.
- Preserve quiet UI regions.
- Avoid unnecessary focal objects.
- Avoid excessive detail.
- Do not add characters unless they are intentionally part of the design.

For UI elements:

- Prioritize readability and function.
- Remember the game will often be viewed on relatively small mobile screens.
- Do not sacrifice usability for decoration.
- Keep visual hierarchy obvious.

---

# 13. Codex's Role as Art Director

Codex should behave as an **iterative art director**, not simply as an asset generator.

Before generating or modifying major visual elements:

1. Inspect the relevant existing game screen and assets.
2. Understand what the screen actually needs to accomplish.
3. Consider the visual hierarchy.
4. Determine which elements should be artwork and which should remain UMG/UI.
5. Consider reuse across other screens.
6. Consider mobile readability and landscape composition.
7. Generate or propose artwork based on those constraints.
8. Evaluate the result in the actual game when practical.

Do not redesign unrelated systems merely because they could be redesigned.

Do not make large stylistic assumptions from a single asset.

---

# 14. Iteration Philosophy

The art direction is still being developed.

The correct workflow is therefore:

**explore → evaluate → refine → establish → document**

rather than:

**generate once → declare final**

When presenting significant new visual directions, prefer a small number of meaningfully different alternatives rather than many minor variations.

Explain what each exploration is testing.

Examples:

- Different background composition
- Different amount of environmental detail
- Different UI material treatment
- Different title treatment
- Different balance between illustration and graphic UI

Once the user strongly prefers a direction, refine that direction rather than continuing to introduce unrelated alternatives.

---

# 15. Approved vs. Experimental Artwork

Do not treat every generated image as part of the game's established visual identity.

Artwork should conceptually fall into three states:

### Experimental

An exploration intended to test an idea.

### Promising

The user likes the general direction, but further iteration is expected.

### Approved

The user has explicitly accepted the asset or direction as something that should define the game's visual language.

Only **approved** artwork should be treated as authoritative style reference material.

Currently, the landscape background exploration should be considered **Promising**, not Approved/Final.

---

# 16. Maintaining This Document

This document should evolve as the visual identity becomes established.

When an important art-direction decision is made, update this document so future sessions do not need to reconstruct that decision from conversation history.

Examples include:

- Final background direction
- Final color palette
- Button style
- Panel style
- Typography
- Happiness logo treatment
- Selected-screen composition conventions
- Transition/VFX style
- Puzzle-board visual treatment
- Approved asset examples

Do not fill these sections with speculative rules before those decisions have actually been made.

The goal is to document **decisions**, not prematurely constrain future exploration.

---

# 17. Immediate Next Step

Continue from the current background exploration.

The most recent direction is a **landscape-oriented, bright illustrated environment with significantly reduced visual clutter and generous open areas for UI**.

It is promising, but not final.

The next task should be to evaluate that background in the context of the actual Happiness title/menu screen and determine:

- Whether the overall environmental concept feels appropriate
- Whether it is still too busy
- Whether UI readability is sufficient
- Whether foreground framing is useful or distracting
- Whether the palette works with the existing puzzle icon artwork
- Whether the image needs to be more abstract/general-purpose
- How well it can be reused on other menu screens
- What additional background variations would be useful
- How it behaves at the game's actual landscape resolutions/aspect ratios

Do not immediately replace the current direction.

First inspect it in context, identify specific strengths and weaknesses, and iterate deliberately from there.

---

# Core Principle

The visual identity of **Happiness** should be recognizable because of its **quality, illustration style, color, warmth, clarity, and consistency** — not because it depends on a narrow theme.

The game should feel like a pleasant place to spend time solving puzzles.
