# TagManager SPINE1 Scoped Loose Tags (2025-12-17 15:00)

What changed
- Added Blueprint handle type FSOTS_LooseTagHandle (Public) with IsValid and Make helper.
- USOTS_GameplayTagManagerSubsystem now maintains scoped handle records and per-actor scoped counts, exposes scoped APIs (AddScopedTagToActor/AddScopedTagToActorByName/RemoveScopedTagByHandle) plus SPINE1 aliases (AddScopedLooseTag/AddScopedLooseTagByName/RemoveScopedLooseTagByHandle), and binds EndPlay to clean scoped data/handles.
- Added Blueprint-assignable events OnLooseTagAdded/OnLooseTagRemoved (FSOTS_OnLooseTagChanged) firing on scoped visibility transitions (count 0->1 / 1->0; visibility-aware with existing union logic).
- TagLibrary now exposes scoped helpers and SPINE1 aliases with world-context forwarding.

New APIs
- FSOTS_LooseTagHandle (BlueprintType)
- Subsystem: AddScopedTagToActor, AddScopedTagToActorByName, RemoveScopedTagByHandle
- Subsystem aliases: AddScopedLooseTag, AddScopedLooseTagByName, RemoveScopedLooseTagByHandle
- Events: OnLooseTagAdded, OnLooseTagRemoved (FSOTS_OnLooseTagChanged)
- TagLibrary: AddScopedTagToActor, AddScopedTagToActorByName, RemoveScopedTagByHandle, plus scoped loose aliases (AddScopedLooseTag*, RemoveScopedLooseTagByHandle)

Event semantics
- OnLooseTagAdded fires when a scoped tag count transitions 0 -> 1 (visible in union).
- OnLooseTagRemoved fires when a scoped tag count transitions 1 -> 0 (visible scope removed); removal suppressed if tag still visible via other sources.

EndPlay backstop (SPINE1)
- EnsureEndPlayBinding binds once per actor; HandleActorEndPlay clears scoped counts, removes related handles, unbinds, and broadcasts removals for visible tags.

Deferred / SPINE2 TODOs
- Broader “no bypass” migration to route other plugins through TagManager/TagLibrary.
- Further tuning of union semantics or additional destroyed hooks if needed.

Files touched
- Plugins/SOTS_TagManager/Source/SOTS_TagManager/Public/SOTS_LooseTagHandle.h
- Plugins/SOTS_TagManager/Source/SOTS_TagManager/Public/SOTS_GameplayTagManagerSubsystem.h
- Plugins/SOTS_TagManager/Source/SOTS_TagManager/Private/SOTS_GameplayTagManagerSubsystem.cpp
- Plugins/SOTS_TagManager/Source/SOTS_TagManager/Public/SOTS_TagLibrary.h
- Plugins/SOTS_TagManager/Source/SOTS_TagManager/Private/SOTS_TagLibrary.cpp

Verification
- UNVERIFIED (no build/editor run).

Cleanup
- Plugins/SOTS_TagManager/Binaries removed.
- Plugins/SOTS_TagManager/Intermediate removed.
