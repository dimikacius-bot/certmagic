# Zombie Survival (Unreal Engine 5.5)

Survie apocalyptique en monde ouvert : parkour à la première personne façon *Dying Light*, zombies qui
se multiplient la nuit, **économie de récupération** (butin, achat et revente d'armes à des PNJ dans des
zones sûres), et une architecture pensée pour une **carte gigantesque** (World Partition + zombies
streamés autour du joueur).

> **État honnête du projet**
> - C'est une **base de gameplay en C++**, écrite sans éditeur Unreal à disposition : le code n'a
>   **pas encore été compilé ni lancé**. Attends-toi à corriger quelques erreurs de compilation au
>   premier build (voir « Premier lancement »).
> - **Aucun asset artistique n'est fourni.** Joueur, zombies, PNJ et objets sont des primitives
>   colorées (cylindres/cubes). Le rendu « The Last of Us Part I Remastered » vient des assets, de
>   l'éclairage et du post-process, pas du code : la config Lumen/Nanite/VSM est prête, le contenu reste
>   à importer (voir [docs/WORLD_SETUP.md](docs/WORLD_SETUP.md)).
> - La carte n'existe pas encore : un `.umap` est un fichier binaire qui se construit dans l'éditeur.

## Prérequis

- Unreal Engine **5.5** (Epic Games Launcher). Autre version 5.x : clic droit sur `ZombieSurvival.uproject`
  → *Switch Unreal Engine version…*
- Windows 10/11 + Visual Studio 2022 avec les charges de travail *Développement de jeux avec C++* et
  *Windows SDK*.
- GPU DirectX 12 / Shader Model 6 (requis par Lumen, Nanite, Virtual Shadow Maps).

## Premier lancement

1. Clic droit sur `ZombieSurvival.uproject` → **Generate Visual Studio project files**.
2. Ouvre le `.sln`, configuration **Development Editor / Win64**, puis **Build**.
3. Lance le projet. Corrige les éventuelles erreurs de compilation (le code n'a jamais été compilé).
4. Crée un niveau de test en 2 minutes :
   - *File → New Level → Basic* (ou *Open World* pour la vraie carte).
   - Place un **NavMeshBoundsVolume** très grand (scale ≈ 500×500×50). Le navmesh n'est construit qu'autour
     du joueur (invoker), le volume définit juste la zone autorisée.
   - Depuis *Place Actors* : **ZSSafeZone** (boîte verte), 2 ou 3 **ZSMerchantNPC** dedans (choisis le
     `Merchant Type` : General, Gunsmith, Medic, Fence), quelques **ZSLootContainer** dehors (change la
     `Loot Table`), éventuellement une **ZSDangerZone**, et un **PlayerStart**.
   - Le sol doit avoir une collision. `ZSGameMode` est déjà le mode de jeu par défaut (voir
     `Config/DefaultEngine.ini`).
   - **Play.** Les zombies apparaissent d'eux-mêmes autour de toi (à 30–60 m, hors de vue).

## Contrôles

| Action | Touche |
|---|---|
| Déplacement / regard | ZQSD (WASD) / souris |
| Sprint (consomme l'endurance) | Maj gauche |
| Accroupi | Ctrl gauche / C |
| Saut, **franchir un obstacle, grimper une corniche** | Espace (maintenu contre un mur) |
| Attaquer | Clic gauche (maintenu = enchaîne) |
| Recharger | R |
| Changer d'arme (poings → armes de l'inventaire) | Molette / X = ranger |
| Soigner (meilleur objet médical pour les PV manquants) | H |
| Interagir : fouiller, ramasser, commercer | E |
| Inventaire | I |
| Commerce | ↑/↓ choisir, **Entrée** échanger 1, **Maj+Entrée** échanger 5, **Tab** changer de colonne, E / Retour arrière quitter |

## Économie

- **Argent** : caisses fouillables (`ZSLootContainer`, 6 tables de butin), billets au sol, zombies (petite
  chance de drop), revente d'objets. Tu commences avec 120 $, un tuyau, 2 bandages et une conserve.
- **Marchands** (`ZSMerchantNPC`, dans une zone sûre) :

  | Type | Achète | Vend | Trésorerie |
  |---|---|---|---|
  | General Store | déchets, matériaux, nourriture, médical, mêlée | soins, nourriture, ferraille, armes de mêlée basiques | 700 $ |
  | Armory | armes à feu, munitions, mêlée | pistolet, fusil à pompe, fusil de chasse, munitions, machette, hache | 1500 $ (max 3000) |
  | Field Clinic | médical, nourriture | bandages, kits de soin, antibiotiques | 900 $ |
  | The Fence | **tout**, au meilleur taux (50 %) | bijoux, montres, électronique, radios (×1,6) | 2000 $ (max 4000) |
  | Custom | selon ta config | selon ta config | selon ta config (aucune valeur par défaut appliquée) |

- **Prix d'achat** = valeur × marge du marchand × modificateur régional × état (+15 % si presque en rupture).
- **Prix de revente** = valeur × taux de rachat × modificateur régional × **état de l'arme** (35 % à 0 %
  de durabilité, 100 % à neuf) × **demande** (plus le marchand a déjà l'objet en stock, moins il le paie).
- Un marchand ne peut payer que ce qu'il a en caisse, n'achète que ce qui l'intéresse, et **ce que tu lui
  vends rejoint son stock** (rachetable, avec marge). Chaque jour : la caisse se remplit un peu, le stock
  de base est réapprovisionné, une partie des objets revendus disparaît.
- **Mort** : 20 % de ton argent reste au sol où tu es tombé (récupérable) ; tu te réveilles à la dernière
  zone sûre visitée, inventaire conservé.
- Tout est réglable dans *Project Settings → Game → Zombie Survival* et sur chaque marchand.

## Zombies et monde

- **Types** : Walker (lent), Runner (rapide, fragile), Brute (tank), Volatile (la nuit seulement,
  ~ta vitesse de sprint, voit très loin).
- **IA** en code (pas d'asset Behavior Tree) : errance → enquête sur un bruit → poursuite à vue → attaque.
  Les **armes à feu et le combat font du bruit** (portée par arme, dans `FZSItemRow::Noise`) : un coup de
  fusil attire tout ce qui est à portée. Les tirs à la tête font ×2,5.
- **Jour/nuit** (`UZSWorldDirector`) : 40 min réelles par jour (réglable). La nuit : budget de zombies
  doublé, Volatiles, vision étendue. `ZSSkyController` anime soleil/lune et recapture le ciel.
- **Zones sûres** (`ZSSafeZone`) : personne ne t'y poursuit, rien n'y apparaît, les gardes abattent les
  zombies qui y entrent, point de réapparition.
- **Zones dangereuses** (`ZSDangerZone`) : multiplient (ou réduisent) la densité de zombies.
- **Carte géante** : aucun zombie n'est posé à la main. Le directeur en maintient un budget autour de
  chaque joueur (spawn hors de vue, despawn au loin), le navmesh n'existe qu'autour du joueur
  (Navigation Invoker), et les zones s'enregistrent seulement quand elles sont chargées (compatible
  World Partition).

## Architecture du code

```
Source/ZombieSurvival/
  Core/       ZSWorldDirector (horloge, zones, population)  ZSGameMode  ZSSkyController
              ZSSettings (Project Settings)  ZSInteractable  ZSPlaceholder
  Inventory/  ZSItemTypes  ZSItemSubsystem (catalogue en code + DataTable optionnelle)  ZSInventoryComponent
  Economy/    ZSMerchantComponent (prix, achat/vente)  ZSMerchantNPC  ZSSafeZone
              ZSLootTables  ZSLootContainer  ZSPickup
  Combat/     ZSHealthComponent  ZSCombatComponent (mêlée, tir, recharge, usure, bruit)
  Player/     ZSCharacter (parkour, endurance)  ZSPlayerController  ZSHUD (Canvas, sans asset)
  AI/         ZSZombie  ZSZombieAIController  ZSDangerZone
Config/       DefaultEngine.ini (Lumen/Nanite/VSM, navmesh dynamique)  DefaultInput.ini  DefaultGame.ini
docs/         WORLD_SETUP.md — carte géante, graphismes, personnages
```

Points d'extension : les classes sont **sous-classables en Blueprint** (mesh squelettique, AnimBP, sons).
`AZSZombie` expose `OnAttackStarted / OnHitReaction / OnDied`, `AZSCharacter` expose `OnMantleStarted / OnDied`,
`UZSCombatComponent` expose `OnAttack / OnReloadStarted / OnWeaponBroke`. Pour ajouter ou modifier des objets
sans toucher au C++ : DataTable de type `ZSItemRow` renseignée dans *Item Override Table*.

## Ce qui n'est pas fait

Sauvegarde/chargement, multijoueur (rien n'est répliqué), craft, quêtes, animations et sons, arbres de
compétences, manette (les axes de regard sont configurés pour la souris), menu principal, interface UMG
finale (le HUD Canvas est fonctionnel mais brut), véhicules.
