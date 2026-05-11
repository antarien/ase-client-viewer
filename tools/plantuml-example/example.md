# PlantUML Block — Renderer Demo

Inline-Markdown mit eingebettetem PlantUML-DSL-Block. Der Viewer
soll diesen Block als Diagramm rendern (SVG via `plantuml -tsvg -pipe`),
analog zum bestehenden Mermaid-Renderer (`vwr_rndr_mmrd.cpp`).

## Sequence Diagram

```plantuml
@startuml
skinparam backgroundColor #0A0A0A
skinparam sequence {
    ArrowColor #5A9CB8
    LifeLineBorderColor #5A9CB8
    ParticipantBorderColor #5A9CB8
    ParticipantBackgroundColor #1A1A1A
    ParticipantFontColor #8A9A9A
}

actor Client
participant "ase-server-game" as Server
database "Neo4j" as DB

Client -> Server : POST /api/player/spawn
Server -> DB : MATCH (p:Player)
DB --> Server : Player node
Server --> Client : 200 OK { entity_id }
@enduml
```

## Component Diagram

```plantuml
@startuml
skinparam backgroundColor #0A0A0A
skinparam componentStyle rectangle
skinparam component {
    BackgroundColor #1A1A1A
    BorderColor #5A9CB8
    FontColor #8A9A9A
}

[ase-terrain] --> [ase-ecs]
[ase-network] --> [ase-ecs]
[ase-player]  --> [ase-ecs]
[ase-ecs]     --> [ase-math]
@enduml
```
