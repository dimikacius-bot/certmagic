# Carte géante, graphismes et personnages

Ce que le code ne peut pas faire à ta place : construire la carte et importer les assets. Voici la marche à
suivre dans l'éditeur. Les valeurs sont des **points de départ à ajuster**, pas des réglages validés.

## 1. Créer la carte (World Partition)

1. *File → New Level → **Open World***. Enregistre-la en `Content/Maps/Wasteland`, puis dé-commente
   `GameDefaultMap` / `EditorStartupMap` dans `Config/DefaultEngine.ini`.
2. **Paysage** : mode *Landscape → Manage → New*. Une résolution de **8129 × 8129** (127 × 127 composants)
   avec une échelle X/Y de **200** donne ≈ **16 × 16 km** (256 km²). En sprint (6,5 m/s), traverser prend
   ~40 min. À l'échelle 100 (1 m par sommet) : 8 × 8 km avec plus de détail au sol. Génère le relief hors de
   l'éditeur (Gaea, World Machine, World Creator) et importe la heightmap 16 bits.
3. **World Settings → World Partition Setup** : taille de cellule du *Runtime Grid* 25 600 (256 m) au départ,
   *Loading Range* 51 200 – 76 800 pour les bâtiments. Active le **HLOD** pour la vue lointaine et un
   paysage **Nanite** si ta version le propose.
4. **Navigation** : place un **NavMeshBoundsVolume** couvrant tout le paysage. Grâce à
   `bGenerateNavigationOnlyAroundNavigationInvokers` (déjà dans `DefaultEngine.ini`) et au composant
   *Navigation Invoker* du joueur, seul le voisinage du joueur (rayon 70–90 m) est maillé : le coût ne
   dépend pas de la taille de la carte.
5. **Contenu de jeu** (à placer ou à générer par PCG) :
   - `ZSSafeZone` + `ZSMerchantNPC` : un camp tous les 2–3 km, avec des `Merchant Type` variés.
     Ajuste `Region Price Modifier` (> 1 en zone isolée, < 1 dans un hub commercial).
   - `ZSLootContainer` (ou une sous-classe Blueprint) dans les bâtiments, avec la `Loot Table` adaptée :
     Residential, Medical (hôpitaux), Police, Military, Industrial.
   - `ZSDangerZone` sur les zones à risque (hôpital, centre-ville, base militaire) : densité ×2 à ×3.
   - Ne **place pas de zombies à la main** : `UZSWorldDirector` les génère autour du joueur.
6. **Débit de génération** avec le **PCG Framework** (plugin déjà activé dans le `.uproject`) : graphes qui
   posent bâtiments, routes (splines), épaves, végétation et points de butin sur des zones de la carte.

## 2. Ciel, lumière et jour/nuit

Dans le niveau : *Sky Atmosphere*, *Volumetric Cloud*, *Sky Light* (Real Time Capture), *Exponential Height
Fog* (Volumetric Fog activé) et une *Directional Light* dont la **Mobility = Movable** avec
*Atmosphere Sun Light* coché. Ajoute un **ZSSkyController**, assigne-lui la Directional Light et la Sky
Light : il pilote soleil, lune, couleur et intensité selon l'horloge de `UZSWorldDirector`.

## 3. Le look « The Last of Us Part I Remastered »

Le moteur fournit la technologie (déjà activée dans `Config/DefaultEngine.ini` : Lumen GI + réflexions,
Nanite, Virtual Shadow Maps, TSR, DX12/SM6). Le rendu vient ensuite de :

- **Assets** : Megascans et packs post-apocalyptiques via **Fab** (bâtiments, débris, végétation, décals).
  Nanite pour le décor, matériaux à *Runtime Virtual Texture* pour fondre les objets dans le terrain.
- **Palette** : végétation qui reprend le dessus, lumière rasante, ombres douces, brume au sol. Évite les
  couleurs saturées.
- **Post Process Volume (Infinite Extent)** — valeurs de départ : Bloom modéré (~0,4), saturation ~0,85,
  contraste ~1,05–1,1, léger grain de film (~0,1), vignettage (~0,3), aberration chromatique très faible,
  occlusion ambiante ~0,6, auto-exposition avec plage EV100 étroite pour éviter les sauts de luminosité.
- **Personnages** : **MetaHuman Creator** pour le joueur et les PNJ ; zombies à partir de MetaHumans
  abîmés (textures, décals, cheveux) ou de packs Fab. Squelette Manny/Quinn + **IK Retargeter** pour réutiliser
  les animations du projet d'exemple d'animation d'Epic.
- **Animation** : *Motion Matching* + AnimBP pour le parkour ; branche-les sur les événements
  `OnMantleStarted`, `OnAttackStarted`, etc. (voir « Brancher le contenu » ci-dessous).
- **Performance** : une carte de cette taille avec ce rendu vise du matériel récent. Surveille
  `stat unit`, `stat gpu`, la mémoire de streaming (`r.Streaming.PoolSize`) et le budget de zombies.

Réalisme de l'attente : *The Last of Us Part I* est un jeu de studio AAA avec des centaines d'artistes.
Unreal 5 permet d'en approcher la technologie ; l'écart se joue sur la quantité et la qualité des assets.

## 4. Brancher le contenu dans le C++

1. **Zombie** : crée `BP_ZSZombie` (parent `ZSZombie`), assigne un Skeletal Mesh + AnimBP au composant
   *Mesh* (l'objet de remplacement se masque tout seul), gère `OnAttackStarted` / `OnHitReaction` / `OnDied`.
   Renseigne-le dans *Project Settings → Game → Zombie Survival → Zombie Class*.
2. **Joueur** : crée `BP_ZSCharacter` (parent `ZSCharacter`) avec bras/arme en première personne, puis
   `BP_ZSGameMode` (parent `ZSGameMode`) avec ce pawn comme *Default Pawn Class*, et fais-en le mode de jeu
   du niveau (*World Settings → GameMode Override*).
3. **Marchands** : `BP_ZSMerchantNPC` (parent `ZSMerchantNPC`) avec un mesh de personnage.
4. **Objets** : ajoute des icônes/meshes en étendant `FZSItemRow` (DataTable *Item Override Table*).
5. **Interface** : le HUD Canvas (`ZSHUD`) est un placeholder ; remplace-le par des widgets UMG qui lisent
   les mêmes getters publics (`AZSCharacter`, `UZSInventoryComponent`, `UZSMerchantComponent`).

## 5. Utiliser le plugin Unreal dans Claude Code (en local)

Le plugin **Unreal Engine Skills** pilote un éditeur Unreal **ouvert sur ta machine**, via MCP. Il ne
fonctionne donc pas depuis une session cloud, mais dès que le projet est ouvert dans l'éditeur, lance
Claude Code en local : c'est le bon outil pour poser les acteurs, régler le niveau, créer les Blueprints
dérivés et tester en éditeur.
