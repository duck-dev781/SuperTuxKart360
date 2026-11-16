# STK track data and first-pass kart motion

## Black Forest data inspected

The official `tracks/black_forest` assets at SVN revision **18627** include:

- `black_forest_track.spm`: static track geometry, 64 material groups, 64 submeshes, 61,227 vertices, and 145,743 indices.
- `quads.xml`: 1,807 drive quads. The file uses both explicit XYZ points and references to earlier quad points in the form `quad-index:point-index`.
- `graph.xml`: a node list covering quads 0-1806, a main loop from quad 0 to 1054, and shortcut edges/edge-lines.
- `track.xml`: track name `Black Forest`, two default laps, reverse direction available.

The CI asset-validation workflow exports these official files at the recorded revision, parses the SPM mesh, and parses the quads/graph/track metadata.

## TrackDataReader

`STK/Source/TrackDataReader.cpp` is a small C++03 parser for the parts of the official XML needed for an initial driving prototype:

- `quads.xml`: height-testing limits, quad corners, references to prior quad corners, and basic invisible/AI-ignore flags.
- `graph.xml`: `node-list`, `edge-loop`, `edge-line`, and `edge` connections.
- `track.xml`: track name, default lap count, and reverse availability.

It resolves quad point references as the upstream format expects and provides a point-in-quad test for the drive surface. It is not a full replacement for STK's DriveGraph/Track classes and does not load scene objects, track checks, item placements, or AI racing lines.

## KartDynamics

`STK/Source/KartDynamics.cpp` is an early arcade movement model used to validate controller-driven movement against the parsed drive quads:

- left-stick steering;
- right-trigger acceleration;
- left-trigger braking/reverse;
- speed clamping and basic drag;
- track-quad containment, with speed loss when the kart attempts to leave the drivable surface;
- basic main-loop crossing count.

This is **not** STK's production kart physics. It does not yet implement Bullet integration, suspension, tire forces, drifting, jumping, collision response, items, AI, or the original race rules. The next step is to connect a real STK kart model, track mesh, and camera to the Xenos renderer, then replace the prototype motion with the upstream physics system.

## Current test coverage

- Synthetic C++03 tests cover quad-reference resolution, graph loops, track metadata, point-in-quad checks, acceleration, and braking.
- CI has parsed the real Black Forest track XML and confirmed 1,807 quads, main loop 0-1054, two laps, and reverse availability.
- The actual Xbox 360 XEX still needs to load these files and be verified on hardware before this work can be called playable.
