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

The final visual treatment for the **Happiness** title/logo has not yet been established.

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