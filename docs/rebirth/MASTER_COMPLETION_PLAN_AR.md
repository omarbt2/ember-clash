# EMBER CLASH REBIRTH — MASTER COMPLETION PLAN

| Field | Value |
|---|---|
| Purpose | مرجع التنفيذ الموحد لمراحل P0–P14 ومتطلبات Gamepad واللعب المحلي. |
| Status | ACTIVE DRAFT - لا يمثل اكتمال أي مرحلة. |
| Revision date | 2026-11-25 |

**النسخة:** 1.0 — مسودة خطة التنفيذ الرئيسية  
**التاريخ:** 25 نوفمبر 2026  
**الحالة:** خطة مقترحة للمراجعة والاعتماد؛ ليست إعلاناً عن اكتمال أي مرحلة  
**المستودع:** `omarbt2/ember-clash`  
**التقنية الأساسية:** C++20 + SDL3 + CMake  

---

## 0. الغرض وقواعد المشروع

هذه الوثيقة هي نقطة الرجوع الواحدة لإكمال Ember Clash Rebirth في هذه المحادثة وفي جلسات التطوير اللاحقة. تحتفظ بأسماء المراحل P0–P14 الموجودة في Production Bible، وتضيف تفصيلاً تنفيذياً لمسار Gamepad واللعب المحلي المطلوبين.

### قواعد لا نتجاوزها

1. **لا نبدأ من الصفر:** نحتفظ بالتقنية والكود والاختبارات الموجودة، ونصلح الفجوات فوقها.
2. **لا نخلط النسخ:** لا ندمج ملفات من أرشيفين قبل مقارنة commit ancestry والـdiff واختبار النتيجة.
3. **لا نعلن النجاح من قراءة الكود فقط:** كل مرحلة تحتاج اختباراً آلياً، وبناءً فعلياً، ودليلاً يثبت ما تم.
4. **لا نعلن دعم Gamepad من وجود Gamepad.cpp:** الدعم لا يصبح VERIFIED إلا بعد التحكم الفعلي بالمقاتل واختبار عدة أجهزة، بما فيها الفصل وإعادة التوصيل.
5. **لا نستخدم رسومات مؤقتة في إصدار نهائي:** الـprocedural sprites الحالية أساس مؤقت؛ كل شخصية وساحة وأثر وصوت يجب أن يمر عبر إنتاج الأصول والتحقق والترخيص.
6. **لا نكسر master ولا نحذف تاريخ Git:** التطوير على فروع مستقلة، commits صغيرة، ولا force-push.
7. **كل خيار في القائمة يجب أن يعمل:** لا عناصر واجهة وهمية أو أزرار تفتح صفحة غير موجودة.
8. **الجودة هدف قابل للقياس:** لن ندّعي أن اللعبة «أفضل من كل الألعاب» بمجرد الوصف؛ نثبت الجودة عبر الإحساس بالتحكم، توازن القتال، وضوح الرسوم، الأداء، واختبارات اللاعبين.

### تعريف الحالة

- `VERIFIED`: اختُبر فعلياً مع دليل قابل لإعادة التشغيل.
- `PARTIALLY VERIFIED`: توجد أجزاء تعمل، لكن المسار الكامل أو الأجهزة الحقيقية لم تُختبر.
- `UNVERIFIED`: لم يُختبر أو لا يوجد دليل كافٍ.
- `MISSING`: الميزة أو المحتوى غير موجود.
- `BROKEN`: توجد مشكلة معروفة تمنع الاستخدام الصحيح.

---

## 1. ما الذي وجدناه في الاستعادة؟

### 1.1 هوية المشروع الحالية

Ember Clash لعبة قتال منصات جانبية 2D أصلية، بأسلوب damage-and-knockback وstocks وring-outs، وليست لعبة 3D. المشروع مبني بـC++20 وSDL3 وCMake دون محرك ألعاب تقليدي. الهدف أن يكون القتال سريعاً وواضحاً، مع أولوية اللعب المحلي، ثم الشبكة في مرحلة لاحقة.

### 1.2 مصادر الاستعادة التي يجب التوفيق بينها

| المصدر | ما يحتويه | التعامل الصحيح |
|---|---|---|
| `ember-clash-rebirth.bundle` | فرع التطوير `dev/rebirth-continue` عند `2eeceaea2452164e4fe8c064d8e0572ce6678b5e`، وفرع الاستعادة ومرجع master | مرجع التطوير البرمجي الأساسي الذي يجب إعادة التحقق منه قبل أي دمج |
| `rebirth-production-bible` bundle | وثائق الخطة عند `d48fe96751594295f952c567c32353963ed2c4e6` مبنية على مصدر `2eeceae` | المرجع التصميمي والإنتاجي؛ ندمجه داخل `docs/rebirth/` بعد التحقق |
| مجلد Recovery Worktree | لقطة عند `bd71c89` مع تعديلات غير ملتزمة في القتال وملف screenshot stub | مصدر مقارنة/استعادة فقط؛ لا ننسخ تعديلات الملفات عشوائياً فوق فرع التطوير |
| GitHub `master` | آخر رأس تمت معاينته في الاستعادة `365348a2dacea4e03362bfd1fd718582361ba693` | لا نكتب عليه مباشرة؛ نعيد نشر فرع التطوير الموثوق بعد التحقق |

**ملاحظة مهمة:** وجود `dev/rebirth-continue` داخل حزمة Git لا يثبت وحده أن الفرع منشور حالياً على GitHub. مرحلة P0 ستتحقق من ذلك وتعيد نشره على فرع واضح إن لزم. توجد كذلك فروع/تقارير استعادة إضافية أُنشئت في جلسات سابقة؛ لا نعتبرها بديلاً عن مقارنة الشيفرة الفعلية.

### 1.3 الموجود فعلياً حسب Baseline Audit والملفات المصدرية

| النظام | الحالة الحالية الموثقة | ما ينقص |
|---|---|---|
| البنية والتقنية | C++20/SDL3/CMake؛ بنية Core/Gameplay/Physics/UI/Input/Graphics/Audio | تثبيت خط التطوير واستمرار اختبارات جميع المنصات |
| حلقة اللعب | P17 أضاف fixed-step accumulator عند 60Hz وvsync وكاميرا أقرب | لا يوجد render interpolation؛ بعض مؤقتات اللعب ما زالت بالثواني `double`؛ إغلاق determinism gate ما زال مطلوباً |
| الحركة | حركة أساسية وقفزتان؛ `step_motion` موجود | jump buffer وcoyote time وrun/dash وdrop-through وledge وDI وblast zones كاملة غير منجزة |
| القتال | 16 AttackId؛ hitboxes أساسية؛ guard/shield وparry وdodge وEmber Surge | grabs/throws وprojectiles وtraps وarmor وpriority/clank وstaleness وDI المتقدم ونظام move-data كامل |
| الشخصيات | مقاتلان في `Match`؛ أساس بيانات/تصميم للشخصيات الثماني | شخصيتان بميكانيكيات فريدة ومكتملة، ثم التوسع إلى أربع وثمانٍ |
| AI | FSM أساسي: Idle/Approach/Attack/Retreat/Recover/Defend؛ Easy/Standard/Hard | Expert، personalities، perception rules، multi-opponent/team logic، Training Mode |
| الساحات | Arena واحدة مبسطة: أرضية رئيسية ومنصتان جانبيتان، وkill plane سفلي | 3 ساحات إطلاق كاملة، ثم 6 إجمالاً؛ blast zones وحواجز/كاميرا ومخاطر مدروسة |
| الرسوم والتحريك | Sprite إجرائي صغير 64x64 وعدد قليل من poses/frames؛ ليست أصولاً فنية إنتاجية | شخصيات مصممة فعلياً، rigs/atlases، حركات كل هجمة ودفاع ويد وسلاح، VFX وساحات مرسومة |
| الصوت | مولّد صوت إجرائي وتجارب Audio؛ الاختبارات headless لا تثبت السماع | SFX وموسيقى أصلية، loader ومكساج، واختبار تشغيل مسموع على أجهزة حقيقية |
| Gamepad | كلاس لجهاز واحد، بعض تنقل القوائم وrumble؛ ربط المقاتل غير مثبت | نظام متعدد الأجهزة، input mapping لكل لاعب، صفحة Controller Setup، اختبارات أجهزة فعلية |
| القوائم | Title/Main/Play/Pause/Rematch/Settings/Controls/Quit وبعض rebinding | اختيار نمط/لاعب/شخصية/فريق/ساحة، إعدادات كاملة، نتائج، Training UI، ملف لاعب |
| Local Multiplayer | `Match::kFighterCount = 2`؛ اختبار مدخلَي keyboard في Match | 3–4 لاعبين، أجهزة مستقلة، slot manager، join/leave، فرق وFFA، disconnect/reconnect |
| WebAssembly | ملفات JS/WASM موجودة مسبقاً | إعادة بناء من المصدر الحالي واختبار اللعبة داخل متصفح فعلي |
| Online | `state_hash()` أساس fingerprint فقط | netcode/rollback/relay/rooms/backend كلها مفقودة |
| Build & QA | Baseline Audit يسجل Linux Release سابقاً: 342 خطوة، 0 warnings، و14/14 CTest | إعادة تحقق clean-clone الآن؛ Windows وWASM والأجهزة الحقيقية والشبكة لم تكن مثبتة في Audit |

**لا نخلط بين الاختبارات headless واللعب الحقيقي:** نجاح اختبارات القواعد لا يثبت أن شكل الرسوم جيد، أو أن الصوت مسموع، أو أن يد تحكم فعلية تتحكم في المباراة.

### 1.4 اكتشافات خاصة بالـGamepad

الشفرة الحالية `Gamepad` تحتفظ بمؤشر `SDL_Gamepad*` واحد فقط. كما أن `Match::update` يستقبل `InputManager`، ولا يستقبل مجموعة أجهزة Gamepad لكل لاعب؛ لذلك لا توجد بنية 4 لاعبين بأربع أيدي تحكم بعد.

يوجد أيضاً خلل يجب فحصه وإصلاحه في P8: `InputManager::update()` يستدعي `SDL_PollEvent` ويفرغ طابور أحداث SDL، ثم `Gamepad::update()` يستدعي `SDL_PollEvent` مرة ثانية. وبما أن أحداث التوصيل والفصل قد تكون استُهلكت في الاستدعاء الأول، فإن اكتشاف hot-plug غير موثوق في التصميم الحالي. الحل هو **مالك واحد لطابور الأحداث** يمرر كل حدث للأنظمة المعنية، لا أن تسحب الأنظمة الأحداث من الطابور بصورة مستقلة.

---

## 2. تعريف النطاق النهائي

### 2.1 إصدار الإطلاق الأول V1.0 — Must Have

- 4 شخصيات كاملة: Kaelen Ashforge، Torga Stonehide، Vesper Quill، Isolde Cinderveil.
- 3 ساحات تنافسية مكتملة برسومها وموسيقاها وبيانات التصادم.
- مباراة محلية حتى 4 لاعبين على جهاز واحد.
- Keyboard + Gamepad، أكثر من Gamepad في الوقت نفسه، ومزيج keyboard/gamepads.
- شاشة خاصة لإعداد/اختبار Gamepad، وشاشة انضمام اللاعبين واختيار الشخصية والفريق/الجهة.
- AI بأربع درجات EASY/NORMAL/HARD/EXPERT.
- Training Mode، Versus AI، Local Versus، FFA، Team 2v2، Arcade.
- قوائم وإعدادات وصوت ورسوم وتحريك وأصول نهائية، مع حفظ الإعدادات والإحصائيات المحلية.
- Windows وLinux، وWebAssembly demo من نفس مصدر C++.
- 60 FPS على جهاز المواصفات المرجعية المحدد في الخطة، وعدم وجود crash/soft-lock معروف.

### 2.2 الرؤية الكاملة بعد الإطلاق

- 8 شخصيات إجمالاً: الشخصيات الأربع الأولى، ثم Rook Emberwick وMaren Tidewarden وZephyr Kite وBastian Wardenhold.
- 6 ساحات إجمالاً.
- Online 1v1 غير مصنف أولاً.
- Ranked/Competitive بعد إثبات الاستقرار ونزاهة النتائج.
- إضافات ومحتوى تجميلي فقط؛ لا pay-to-win.

**لا نؤخر إطلاق V1.0 بسبب Online/Ranked:** هذه ميزات Post-launch حسب Production Bible. لكن لا نصف الرؤية الكاملة Rebirth بأنها اكتملت حتى تُنجز مراحل P11–P13 أيضاً.

---

## 3. خارطة التنفيذ P0–P14

> تقديرات الجهد في Production Bible تخص مطوراً واحداً بدوام كامل، وتشمل التطوير والضبط واختبارات المراحل وافتراضات إنتاج الرسوم. هي تقديرات أولية وليست مواعيد مضمونة.

### P0 — Recovery & Audit: توحيد الاستعادة وإثبات نقطة البداية

**الأولوية:** أول شيء نعمل عليه.  
**الحالة:** الاستعادة موجودة، لكن تعدد الفروع والحزم يحتاج تسوية نهائية.

المهام:
1. استنساخ/استعادة `ember-clash-rebirth.bundle` في مجلد عمل جديد، والتحقق من `git bundle verify` و`git fsck` و`git status`.
2. تثبيت `2eeceae` كنقطة مصدر مرشحة، ومقارنته مع `bd71c89` و`7fddfde` وGitHub master؛ لا ندمج أي ملف دون diff واضح.
3. التحقق من commit ancestry والملفات التي أضيفت/حذفت، مع التأكد أن إصلاح 13-key input واختبار `ember_test_rebirth_input` موجودان.
4. دمج تحديثات master غير المتعارضة في فرع استعادة جديد، دون force-push ودون تعديل master مباشرة.
5. إحضار Production Bible إلى `docs/rebirth/`، وتشغيل `tools/check_rebirth_docs.py` وإصلاح الروابط/المراجع إن لزم.
6. إعادة تنفيذ Linux Release build وCTest من clean clone، وتسجيل الأعداد والنتائج الفعلية؛ عدم استعمال أرقام سابقة دون إعادة التحقق.
7. جرد Windows وWASM وaudio وgamepad hardware كحالات غير متحققة إلى أن تُختبر.
8. عزل أو إزالة `tools/ember_screenshot.cpp` إذا ظل placeholder، وعدم تقديمه كأداة حقيقية.

**قبول P0:** مصدر واحد موثق، شجرة نظيفة، تاريخ محفوظ، build/test baseline معاد تشغيله، وقائمة واضحة بكل ما لم يُختبر.  
**المخرج:** `docs/rebirth/evidence/P0_BASELINE.md` + checkpoint commit.

### P1 — Complete Game Design & Technical Plan

**التقدير الأصلي:** 1–2 أسبوع دوام كامل.

المهام:
1. مراجعة واعتماد Production Bible وقرارات D-001 إلى D-020.
2. إغلاق التناقضات بين ما هو موجود في الكود وما هو مقترح فقط.
3. تثبيت نطاق V1.0: 4 Fighters، 3 Arenas، Local 4P، وأنماط اللعب المطلوبة.
4. تثبيت قرارات التوقيت/determinism، snapshot، data format، art pipeline، backend، licensing وGit LFS.
5. تحويل كل acceptance checklist إلى مهام صغيرة قابلة للـcommit والاختبار.

**قبول P1:** قرار موثق لكل قرار OPEN أو PROPOSED، وثوابت موحدة في `00_SHARED_CONSTANTS.md`، ولا تناقضات تمنع بدء P2.

### P2 — Physics & Movement

**التقدير الأصلي:** 5–8 أسابيع.

المهام:
1. تسجيل 20 replay baseline على `2eeceae` قبل إعادة هيكلة الزمن.
2. إغلاق قرار D-001: توحيد gameplay على integer ticks؛ مؤقتات respawn/KO/invulnerability تصبح ticks، بينما interpolation يبقى في renderer.
3. اختبار numeric representation D-002: constrained float مقابل fixed-point candidates، ثم اعتماد الخيار بالأدلة لا بالتخمين.
4. إضافة `kMaxFighters` وتجهيز الفيزياء لعدد 4 مقاتلين.
5. نقل SDL dependencies من simulation إلى طبقة input؛ gameplay يستقبل `InputFrame` موحداً.
6. إضافة jump buffer، coyote time، run/dash، fast fall، drop-through، ledge grabbing/recovery، blast zones الأربع لكل ساحة، وDI/tech.
7. بناء collision no-tunnel fuzz، وتثبيت replay recorder وإكمال snapshot skeleton.
8. اختبار 20 replay بعد التغيير: مواقع الشخصيات ضمن 0.01 px وتطابق ticks أحداث KO/respawn/attack، أو توثيق أي اختلاف بموافقة صريحة.
9. إضافة CI لـLinux وWindows وWASM وفحص determinism حيث تسمح المنصات.

**قبول P2:** جميع حالات الحركة لها tests؛ replay equivalence/determinism gate موثق؛ feel session مع لاعبين اثنين؛ لا تراجع في 60Hz.

### P3 — Combat Engine

**التقدير الأصلي:** 6–9 أسابيع.

المهام:
1. إنشاء نظام data-driven للحركات باستخدام JSON schema وvalidator.
2. دعم hitbox keyframes متعددة لكل حركة، مع shapes وoffsets وactive windows وgroups وrehit rules.
3. استكمال attack state machine: Startup → Active → Recovery → Cancel windows، مع input buffer.
4. إضافة grabs/pummels/throws في الاتجاهات الأربعة، مع counterplay واختبارات mutual-grab.
5. إضافة projectiles: ownership، lifetime، cap، reflect/destroy، interaction بين projectiles، وسلوك المالك عند KO.
6. إضافة traps بوقت arming وlifetime وlimits وقواعد team ownership.
7. إضافة armor وpriority/clank وstaleness وDI والـhitstun/tech rules.
8. تضمين كل state جديد في `MatchState` snapshot وhash؛ لا نضيف state يؤثر على اللعب من دون snapshot completeness test.
9. إنشاء scenario DSL أو harness لاختبارات كل move على tick محدد.

**قبول P3:** frame-exact tests لكل move؛ priority matrix؛ projectile/trap lifecycle tests؛ determinism/snapshot tests؛ review بصري للـhitbox overlay.

### P4 — Two Fully Playable Fighters

**التقدير الأصلي:** 6–10 أسابيع.

الشخصيتان الأوليان:
- **Kaelen Ashforge — Balanced:** Ember/Forge Heat stacks، أسلوب متوازن، specials أصلية.
- **Torga Stonehide — Heavy:** armor/stamina وQuake shockwaves، بطيء وثقيل مع نقاط ضعف قابلة للاستغلال.

المهام:
1. توصيل جميع الحركات الأساسية والـsmashes والـaerials والـspecials من ملفات البيانات إلى اللعب.
2. تنفيذ hooks الفريدة `forge_heat` و`armor_frames` وباقي ميكانيكيات كل شخصية.
3. ضبط الوزن والـknockback والـrecovery والـlanding lag، وليس مجرد تغيير أرقام الضرر.
4. بناء matchup simulator وتشغيل مواجهات AI مقابل AI.
5. جلسات لعب بشرية مع تسجيل الملاحظات وإصلاح exploits والـsoft-lock.

**قبول P4:** كل الحركات قابلة للوصول بالتحكم؛ الشخصيتان مختلفتان في الإحساس والاستراتيجية؛ نتائج المواجهات ضمن النطاق المستهدف 40–60% في اختبارات التوازن المحددة؛ جميع الحالات الخاصة مختبرة.

### P5 — Real Art & Animation Pipeline

**التقدير الأصلي:** 14–26 أسبوعاً.  
**هذه من أهم مراحل المشروع وليست مرحلة تجميل بسيطة.**

#### قرار الأسلوب الفني

الـProduction Bible يترك D-004 مفتوحاً ويطلب تجربة على Kaelen بطريقتين على الأقل:
- A: Blender 3D model + rig + animation، ثم bake إلى sprite sheets 2D.
- B: layered 2D rig ثم bake إلى sprite sheets.
- C: hand-drawn frame-by-frame كخيار جودة قصوى أو للحركات البطولية التي لا تخدمها rigging.

**توصيتي للاختبار:** نبدأ بمقارنة A وB، مع فرضية A: rig ثلاثي الأبعاد يتحرك بحركة جسم وذراعين ويدين حقيقية، ثم نخرجه كرسوم 2D ذات طابع مرسوم/ink-shaded حتى تحافظ اللعبة على هوية 2D. لا نثبت هذا القرار إلا بعد رؤية نموذج داخل اللعبة.

#### معيار الحركة والرسوم الذي نريده

- كل مقاتل له رأس وجذع وكتفان وذراعان وساعدان ومعصمان ويدان وساقان وقدمـان، مع سلاحه/أداته عند الحاجة.
- اليد لا تبقى كتلة ثابتة: grip واضح، anticipation قبل الهجمة، مسار حركة، impact pose، follow-through، ثم recovery.
- كل ضربة يجب أن تحرك الكتف والكوع والرسغ واليد والجذع والقدمين بطريقة مترابطة؛ لا نكتفي بتغيير موضع hitbox مع رسم ثابت.
- الحركات الدفاعية والهروب والهبوط والـKO والـrespawn لها poses واضحة، لا نفس وضعية الوقوف.
- ملامح الوجه، السلاح، القماش/الدروع، silhouette والألوان تميز كل شخصية عند حجم صغير.
- شكل اللعبة العام: hand-forged poster، ink outlines، إضاءة دافئة من الأسفل وظلال باردة، خلفيات متعددة الطبقات؛ مع بقاء الشخصيات أعلى تبايناً من الخلفية.

#### إنتاج الأصول

1. Concept sheet لـKaelen (front/side/back) والـpalette.
2. rig/animation test مع 9 clips على الأقل: Idle, Walk, Run, Jump, Jab, FTilt, FSmash, HitLight, KO.
3. bake لـ`atlas.png` و`anim.json`، وتجربة SpriteSheet loader داخل اللعبة.
4. overlay يبين hitbox/hurtbox على كل clip؛ alignment error لا يتجاوز 2 px.
5. مراجعة 5 أشخاص للقراءة والتمييز؛ silhouette واضح عند 64 px height، والـatlas يحافظ على تفاصيل كافية للحجم المعروض.
6. تسجيل وقت الإنتاج لكل clip وحجم الأصول وسهولة تعديلها.
7. اختيار الطريقة في D-004 ثم تنفيذ Kaelen وTorga بالكامل.
8. إنشاء validator يشيك وجود كل clip، مدة الحركة مع بيانات الهجمة، atlas dimensions، anchors، alpha، وlicense row.
9. Git LFS للأصول المصدرية الكبيرة، مع inventory لكل ملف ورخصته.

#### قائمة التحريك الكاملة لكل شخصية — 47 Clip

| المجموعة | المحتوى |
|---|---|
| Locomotion (11) | Idle, Walk, Run, Turn, Jump, DoubleJump, Fall, FastFall, Land, LedgeHang, LedgeGetup |
| Ground (4) | Jab, FTilt, UTilt, DTilt |
| Smash (6) | FSmashCharge/FSmash, USmashCharge/USmash, DSmashCharge/DSmash |
| Aerial (5) | NAir, FAir, BAir, UAir, DAir |
| Specials (4) | SpecialN, SpecialS, SpecialU, SpecialD |
| Defense (3) | Shield, Parry, Dodge |
| Grab (6) | Grab, Pummel, ThrowF, ThrowB, ThrowU, ThrowD |
| Reactions (7) | HitLight, HitHeavy, Knockback, KO, Respawn, Victory, Defeat |
| Surge (1) | SurgeActivate |

**الحجم:** 47 clip لكل شخصية؛ 4 شخصيات إطلاق = 188 clip؛ 8 شخصيات = 376 clip. هذه الأرقام تشمل الحركات الأساسية والتفاعلات، لا مجرد idle/walk/attack عام.

**قبول P5:** أصول فعلية داخل build؛ كل clip مطلوب موجود؛ اليدان والذراعان تتحركان بشكل مقروء لكل هجمة/دفاع؛ hitbox alignment ≤2 px؛ لا أصول غير مرخصة؛ لا نستخدم procedural placeholder باعتباره فن الإصدار النهائي.

### P6 — Advanced AI & Training

**التقدير الأصلي:** 5–8 أسابيع.

المهام:
1. فصل AI إلى Perception → Situation Model → Utility Scoring → Execution.
2. AI لا يرى مدخلات الخصم القادمة أو RNG؛ يتصرف وفق reaction delay حقيقي.
3. إضافة أربع درجات: EASY/NORMAL/HARD/EXPERT؛ EXPERT لا تقل استجابته عن الحدود الإنسانية المحددة في الوثيقة.
4. إضافة شخصيات AI: Aggressive, Defensive, Technical, Adaptive.
5. FFA/Team targeting دون ضرب الشريك أو dogpiling غير منطقي.
6. إضافة Training Dummy: stand/crouch/shield/attack/recover profiles.
7. إضافة frame data panel، hitbox/hurtbox overlay، input display، position reset، damage reset، record/playback حتى 600 ticks، وتدريبات recovery/edge-guard/punish.
8. تشغيل اختبارات AI-vs-AI ومقارنة ترتيب الصعوبات ونجاح recovery وno-hidden-information.

**قبول P6:** ترتيب الصعوبات صحيح إحصائياً؛ EXPERT قادر على recovery في ≥95% من مجموعة السيناريوهات القياسية المحددة؛ Training flow usable؛ 1000-match fuzz لا يكشف crash/soft-lock.

### P7 — Three Complete Arenas

**التقدير الأصلي:** 5–8 أسابيع.

المهام:
1. إنشاء Arena data loader/schema وblast zones منفصلة للأعلى/الأسفل/اليمين/اليسار.
2. تطوير الكاميرا لتعرض جميع اللاعبين بشكل مقروء، مع حدود وzoom smoothing.
3. إنتاج 3 ساحات كاملة، لكل واحدة هوية معمارية وإضاءة و5 parallax layers على الأقل حسب تصميم الأصول.
4. إضافة collision fixtures وspawn points واختبارات KO/recovery.
5. hazards اختيارية في casual، وتكون OFF في ruleset التنافسي.
6. إضافة arena ambience/music hooks.

**قبول P7:** 3 ساحات مختلفة بصرياً ولعبياً؛ جميع blast zones/spawns/collisions مختبرة؛ إيقاف hazards يعطي قواعد تنافسية ثابتة.

### P8 — Gamepad System & Local Multiplayer (حتى 4 لاعبين)

**التقدير الأصلي:** 4–7 أسابيع للعب المحلي؛ متطلبات Gamepad التفصيلية أدناه إلزامية ضمن P8.

#### A. طبقة أجهزة التحكم

1. إعادة تصميم `Gamepad` من كائن واحد إلى `GamepadManager`/Device Registry يحتفظ بكل الأجهزة المفتوحة.
2. جعل `App` أو Input subsystem المالك الوحيد لـSDL event pump؛ تمرير أحداث add/remove إلى مدير الأجهزة وعدم استعمال `SDL_PollEvent` من أكثر من مكوّن.
3. اكتشاف الأجهزة الموصولة عند بدء اللعبة، وليس انتظار حدث الإضافة فقط.
4. الاحتفاظ بـSDL instance ID للجلسة؛ لا نستخدم instance ID كمعرّف دائم عبر إعادة التشغيل.
5. استخدام SDL3 Gamepad API وmapping database للأجهزة القياسية؛ اختبار Xbox/XInput، PlayStation DualShock/DualSense، Nintendo Pro Controller، وأجهزة USB/Bluetooth عامة عندما يتعرف SDL على mapping صالح.
6. إذا كان الجهاز غير mapped: نظهر رسالة واضحة، ونوفر مسار mapping/إعداد عند الإمكان أو نوضح أن الجهاز غير مدعوم؛ لا ندّعي universal support.
7. توحيد keyboard/gamepad/AI إلى `InputFrame` أو `PlayerInput` واحد يصل إلى Match/Fighter.
8. إضافة deadzone قابل للضبط، trigger handling، axis calibration، repeat delay للقوائم، وbutton edge detection.
9. ربط Haptics بكل controller على حدة؛ اختبار rumble عند hit/KO إذا كان الجهاز يدعمه، مع fallback مرئي/صوتي عند عدم وجود vibration.

#### B. صفحة Gamepad / Controller Setup

تكون موجودة من: `Main Menu → Settings → Input & Controls → Controller Setup`.

المحتوى المطلوب:
- قائمة بكل الأيدي المتصلة: الاسم، نوع الجهاز، حالة الاتصال، واللاعب المعيّن له.
- رسم تخطيطي لليد يُظهر الزر الذي ضغطه اللاعب مباشرةً (Live Button Test).
- اختبار Left/Right Stick والـD-pad والـtriggers والـshoulder buttons.
- شاشة إعادة تعيين الأزرار لكل Action مع كشف التعارض، Swap/Cancel، واستعادة الافتراضي.
- إعداد Deadzone وstick sensitivity، مع اختبار rumble منفصل لكل يد تدعم الاهتزاز.
- تغيير ترتيب أزرار العرض حسب SDL Gamepad type (مثلاً أسماء/رموز الزر) حتى لا تظهر تعليمات Xbox لمستخدم PlayStation بشكل مضلل.
- profiles مستقلة، وحفظ الإعدادات مع device GUID/mapping fingerprint عند الحاجة.
- رسائل عند فصل الجهاز: اسم اللاعب/اليد، reconnect، أو اختيار takeover بالذكاء الاصطناعي/forfeit وفق إعداد المباراة.
- كل صفحة قابلة للاستخدام بالـkeyboard أو Gamepad؛ لا توجد شاشة تحكم لا يمكن الخروج منها إلا بالماوس.

#### C. صفحة انضمام اللاعبين واختيار الجهة

التدفق:

`Mode Select → Player Join → Character Select → Team/Side Select → Stage Select → Match Settings → Match`

- أربع خانات لاعب P1/P2/P3/P4.
- يظهر في كل خانة: جهاز الإدخال، اسم اليد أو Keyboard، الشخصية، الفريق/اللون، وحالة Ready.
- ينضم اللاعب بالضغط على أي زر في جهازه؛ لا يُحتسب الجهاز مرتين ولا يستطيع جهاز واحد التحكم في لاعبين بالخطأ.
- اختيار الشخصية لكل slot بمؤشر مستقل.
- 1v1: P1 يبدأ من الجهة اليسرى وP2 من اليمنى افتراضياً، مع إمكانية تبديل جهة البداية قبل المباراة.
- Team 2v2: اختيار الفريقين مع منع friendly fire افتراضياً.
- FFA: اختيار لون/هوية مستقلة لكل لاعب، وتحديد ترتيب spawn.
- دعم AI slot بدلاً من لاعب بشري، مع اختيار مستوى الصعوبة لكل slot.

#### D. تركيبات الأجهزة التي يجب أن تعمل

| السيناريو | النتيجة المطلوبة |
|---|---|
| Keyboard واحد + Keyboard profile ثانٍ | Local 1v1 على لوحة واحدة، دون تداخل المفاتيح |
| Keyboard + Gamepad | P1 keyboard وP2 gamepad أو العكس |
| Gamepadان | 1v1، كل لاعب يتحكم بيده |
| 4 Gamepads | أربعة لاعبين محليين |
| Keyboard + 3 Gamepads | أربعة لاعبين محليين |
| 2 Gamepads + keyboard profile ثانٍ | أربعة لاعبين إذا كانت slots مستقلة |
| فصل يد أثناء المباراة | pause/reconnect prompt؛ إعادة التوصيل تعود لنفس slot أو takeover وفق الخيار |

**الحد:** أربعة لاعبين متزامنين إجمالاً. الحد الأقصى ليس أربعة gamepads بالإضافة إلى keyboard profiles.

#### E. الاختبارات

- SDL virtual gamepads لاختبارات add/remove/button/axis/reconnect.
- slot-isolation tests تمنع مدخل لاعب من التأثير على لاعب آخر.
- deterministic 4-player replay.
- manual hardware session للأجهزة المتاحة: Xbox، DualShock 4، DualSense، Switch Pro، generic USB/Bluetooth؛ تسجيل نوع الجهاز ونظام التشغيل وطريقة الاتصال.
- اختبار rumble فقط على أجهزة تدعم الاهتزاز.

**قبول P8:** 4 slots تعمل في Match فعلي؛ Controller Setup page مكتملة؛ Join/Side/Team Select مكتملة؛ نجاح جلسة فعلية بأربع أيدي تحكم أو كل التركيبات المتاحة مع توثيق الأجهزة غير المتوفرة؛ لا claim بدعم جهاز لم يُختبر.

### P9 — Full UI & Modes

**التقدير الأصلي:** 8–12 أسبوعاً.

الشاشات:
- Startup/Title وMain Menu.
- Mode Select.
- Player Join.
- Character Select.
- Team/Side Select.
- Stage Select.
- Match Settings.
- HUD كامل: damage، stocks، Surge، أسماء اللاعبين، team markers.
- Pause: Resume/Restart/Settings/Quit.
- Results: الفائز، الإحصائيات، Rematch، العودة للاختيار.
- Settings: Video، Audio، Controls، Gamepad، Accessibility.
- Training Setup وTraining HUD.
- Local Profile/Stats.

الأطوار:
- Versus AI.
- Local Versus 1v1.
- FFA حتى أربعة.
- Team 2v2.
- Arcade.
- Training.
- Survival ضمن Should/مرحلة تالية إن لم يدخل Must.

المهام:
1. إعادة هيكلة Menu state machine ومنع dead ends.
2. ربط كل قائمة بالميزة الحقيقية؛ إخفاء الميزات غير المكتملة.
3. حفظ settings والـprofiles والإحصائيات.
4. Tutorial تفاعلي 90 ثانية، قابل للتخطي، ويدعم prompts خاصة بالـkeyboard أو Gamepad.
5. UX pass بخمسة مختبرين على الأقل.

**قبول P9:** لا زر وهمي؛ كل تدفق يبدأ وينتهي بشكل صحيح؛ keyboard وgamepad يعملان في كل القوائم؛ نتائج/rematch/settings/tutorial تعمل دون restart للتطبيق.

### P10 — Eight-Fighter Roster

**التقدير الأصلي:** 24–42 أسبوعاً؛ الجزء المطلوب للإطلاق الأول 8–14 أسبوعاً.

- Wave A للإطلاق: إضافة Vesper Quill وIsolde Cinderveil فوق Kaelen وTorga.
- Post-launch: Rook Emberwick وMaren Tidewarden وZephyr Kite وBastian Wardenhold.

لكل شخصية:
1. Data/attributes/weight/move table.
2. Unique hook وسلوك specials الأربعة وEmber Surge.
3. جميع 47 animation clips، VFX، SFX، palettes وportrait.
4. Counterplay واضح لكل حركة.
5. Unit tests لكل special/passive والحالات الطرفية.
6. Matchup matrix مع جميع الشخصيات الأخرى، وتحديث التوازن بعد playtests.

**قبول Wave A:** أربع شخصيات مكتملة وقابلة للعب، كل شخصية لها gameplan وهوية بصرية وصوتية، ولا توجد حركة placeholder.

### P11 — Arenas 4–6 (Post-launch)

**التقدير الأصلي:** 5–8 أسابيع.

1. إضافة 3 ساحات casual أخرى.
2. إضافة hazard system مضبوطاً ببيانات كل ساحة، مع إمكانية إيقافه.
3. الحفاظ على التنافسية: hazards OFF يعطي نفس ruleset الثابت.
4. اختبارات تصادم وKO وperformance لكل ساحة.

### P12 — Online 1v1 Unranked (Post-launch)

**التقدير الأصلي:** 14–24 أسبوعاً.

لا يبدأ قبل إغلاق determinism وsnapshot/combat/local release.

1. تحسين save/restore snapshot.
2. input history وrollback حتى 8 frames كهدف أولي، input delay افتراضي 2 frames وفق الوثيقة.
3. transport/relay/rooms/invites/rematch/disconnect/reconnect.
4. netsim harness مع packet loss/latency/jitter.
5. Backend prototype، auth/rooms/match records، حماية أسرار الخادم.
6. 1000-match network simulation واختبارات desync fuzz.
7. اختبار يدوي بين جهازين حقيقيين عند 100ms و150ms.

**قبول P12:** 1000 مباراة محاكاة دون desync غير مفسر، reconnect/disconnect واضح، واللعب بين جهازين يثبت التجربة. إذا فشل rollback، نستخدم delay-based fallback وفق قرار موثق.

### P13 — Ranked & Competitive Features (Post-launch)

**التقدير الأصلي:** 10–18 أسبوعاً.

1. Resimulation worker للتحقق من نتائج المباريات.
2. anomaly detection وabuse tooling.
3. Glicko-2 rating، seasons/leaderboards/profiles.
4. server-side result validation؛ لا نعتمد على العميل وحده لإعلان النتيجة.
5. closed beta واختبارات win trading/abuse.

**قبول P13:** النتائج قابلة لإعادة المحاكاة والتحقق، لا توجد ثغرة معروفة تسمح للعميل بتغيير النتيجة دون اكتشاف، والـranked قابل للإيقاف عند الحاجة.

### P14 — Optimization & Release Preparation

**التقدير الأصلي:** 8–14 أسبوعاً؛ جزء الإصدار الأول 6–10 أسابيع.

1. Profiling على جهاز مرجعي: CPU 4-core من فئة 2018، GPU مدمج، RAM 8GB، دقة منطقية 1280x720.
2. حدود الأداء المرجعية: 16.67ms/frame؛ simulation ≤1.5ms للـ4 fighters؛ render ≤8ms، وتُراجع الأرقام بعد القياس.
3. تحسين تحميل الأصول والـatlases والـVFX والذاكرة.
4. CI build matrix: Windows x64، Linux x64، WebAssembly.
5. Packaging: installer/portable build، runtime dependencies، versioning، crash logs، save migration.
6. Release candidate ثم playtests خارجية وإصلاح P0/P1 bugs.
7. Store assets: icon، screenshots، trailer، description، credits، licenses، privacy/support info عند الحاجة.
8. مراجعة كل قائمة/نمط/شخصية/ساحة من أول التشغيل إلى نهاية المباراة.

**قبول P14:** release builds تنتج من CI؛ كل اختبارات CTest خضراء؛ صفر compiler warnings كهدف؛ 1000-match fuzz بلا crash/soft-lock؛ الأداء عند الهدف المرجعي؛ لا placeholder assets؛ لا ملفات غير مرخصة؛ اختبار مستخدمين خارجيين ناجح.

---

## 4. ترتيب الاعتماد والجدول التنفيذي

### المسار الرئيسي للإصدار V1.0

`P0 → P1 → P2 → P3 → P4 → P5 → P7 → P8 → P9 → P10 Wave A → P14`

أعمال يمكن أن تسير بالتوازي بعد تثبيت العقود التقنية:
- Art spike لـKaelen يبدأ بعد P1 بالتوازي مع P2/P3.
- AI P6 يبدأ بعد P3 بالتوازي مع P5/P7.
- Arena concept/art يبدأ بالتوازي مع اكتمال fighter art pipeline.
- Gamepad UI design يمكن تحضيره أثناء P1، لكن التنفيذ النهائي واختبار 4 players ضمن P8 بعد استقرار InputFrame وMatch slots.

### المراحل اللاحقة

`P11 → P12 → P13` مع الالتزام بشروط الاعتماد المذكورة في كل مرحلة. P12 لا يبدأ قبل أن تصبح snapshots كاملة وقابلة لإعادة المحاكاة.

### تقديرات Production Bible الأصلية

| المرحلة | تقدير دوام كامل لمطور واحد |
|---|---:|
| P0 | 0 أسبوع (تدقيق/استعادة) |
| P1 | 1–2 |
| P2 | 5–8 |
| P3 | 6–9 |
| P4 | 6–10 |
| P5 | 14–26 |
| P6 | 5–8 |
| P7 | 5–8 |
| P8 | 4–7 |
| P9 | 8–12 |
| P10 | 24–42 (جزء الإطلاق الأول 8–14) |
| P11 | 5–8 |
| P12 | 14–24 |
| P13 | 10–18 |
| P14 | 8–14 (جزء الإطلاق الأول 6–10) |

- مجموع النطاق الكامل في الوثيقة: 115–196 أسبوع عمل متسلسل تقريباً.
- أول إصدار موصى به: 68–114 أسبوعاً كجهد متسلسل؛ تقدير التقويم بعد التوازي والاحتياط في الوثيقة 17–28 شهراً بدوام كامل.
- بدوام جزئي يقارب 20 ساعة أسبوعياً: الوثيقة تقدّر أول إصدار بـ38–64 شهراً، والنطاق الكامل بـ65–110 شهراً.

هذه أرقام تخطيطية واسعة وعدم اليقين فيها كبير، خصوصاً مرحلة إنتاج الرسوم والتحريك. لا نستخدمها كموعد تسليم ثابت، ولا نختصرها بادعاء أن AI سيصنع 376 clip نهائياً تلقائياً.

---

## 5. معايير الجودة العامة للعبة

### Gameplay
- كل Action يستجيب ضمن tick واحد من أخذ الإدخال وفق العقد المحدد.
- لا infinite combo أو صفر-إلى-KO غير مقصود.
- لكل حركة counterplay واضح.
- كل مقاتل مختلف في range/weight/specials/tempo، وليس مجرد palette مختلفة.
- توازن الإطلاق يراجع بالـAI simulation وباللعب البشري، ولا يُحسم بإحصاء واحد فقط.

### الرسوم والتحريك
- لا تبقى الشخصيات مربعات ملونة أو أشكالاً مولدة إجرائياً في الإصدار النهائي.
- الأيدي والذراعان والسلاح مرئية ومقروءة في ضربات كل شخصية.
- لا تتقاطع الأطراف بشكل خاطئ، ولا تنزلق القدم عن الأرض أثناء الوقوف/المشي، ولا يتغير موضع hitbox بصورة لا توافق الحركة.
- مؤثرات الضرب لا تغطي الوجه/الجذع لمدة طويلة؛ التباين يسمح بقراءة 4 مقاتلين على الشاشة.

### صوت
- Music/SFX/UI buses منفصلة.
- إعدادات الصوت تتغير فوراً وتُحفظ.
- كل ضربة مهمة وKO وParry وSurge وUI لها feedback مناسب.
- audible playback يختبر على جهاز حقيقي، لا headless فقط.

### أداء/استقرار
- 60Hz simulation؛ render/vsync مستقلان.
- لا desync في replay على المنصات المستهدفة.
- لا crash أو soft-lock في 1000-match fuzz.
- memory/performance budgets موثقة بقياسات حقيقية.

### Accessibility
- color-blind palettes (3)، team shape outlines، text scale 100/125/150%، reduced shake، reduced flashes، subtitles/visual audio cues، one-handed alternate profile، hold/toggle settings.

---

## 6. Git وCI وطريقة العمل بيننا

لكل subtask:
1. نحدد ملفات التغيير والهدف والاختبارات قبل التنفيذ.
2. نعمل على branch مخصص أو branch المرحلة الحالي؛ لا نعمل مباشرة على `master`.
3. ننفذ أصغر تغيير قابل للاختبار.
4. نشغل الاختبارات المناسبة، ثم كامل الاختبارات عند حدود المرحلة.
5. نفحص `git diff` و`git status` ونراجع أي ملف generated أو asset كبير.
6. ننشئ commit برسالة واضحة ونرفع الفرع إلى GitHub بعد التحقق.
7. نحدّث Evidence Matrix وChecklist المرحلة.
8. لا ننشئ tag `rebirth-phase-Pn-done` إلا بعد اكتمال Exit Criteria يدوياً وآلياً.

### بوابات الاختبار الإلزامية

| Gate | الدليل المطلوب |
|---|---|
| G0 Recovery | clean clone + bundles/history + Linux build/CTest + baseline report |
| G2 Determinism | 20 replays + hash equivalence + cross-platform results أو قرار numeric موثق |
| G3 Combat | frame-exact move tests + projectile/trap/priority/snapshot tests |
| G4 Fighters | شخصيتان playable + matchup/balance evidence + playtests |
| G5 Art | art spike + real sprites + 47 clip pipeline + hitbox alignment proof |
| G6 AI/Training | difficulty ordering + no-hidden-info + training workflow |
| G7 Arenas | 3 arenas + collision/KO/spawn/visual review |
| G8 Gamepad/Local | 4 player slots + virtual pad tests + hardware session + disconnect/reconnect |
| G9 UI/Mode | كل التدفقات/الأطوار قابلة للعب دون dead ends |
| G10 Roster | 4 release fighters complete; later 8 with full acceptance |
| G12 Online | 1000 netsim matches + two-machine 100/150ms session |
| G13 Ranked | server-side result validation + abuse tests + closed beta |
| G14 Release | CI artifacts + performance + fuzz + external playtest + licenses |

---

## 7. قرارات ما تزال تحتاج اعتماداً صريحاً

Production Bible يسجل هذه القرارات على أنها PROPOSED أو OPEN أو DEFERRED؛ لا نحوّلها إلى حقائق لمجرد وجودها في الجدول:

- D-001: integer tick simulation وإزالة dt من gameplay مع replay proof.
- D-002: Q16.16 مقابل Q24.8 أو constrained float.
- D-003: POD fixed-array MatchState snapshot.
- D-004: Blender baked sprites مقابل 2D rig أو hand-drawn.
- D-006: 4 fighters للإطلاق الأول.
- D-007: 3 ساحات للإطلاق الأول.
- D-008: إدراج grabs/throws.
- D-009: حدود AI reaction وEXPERT.
- D-010: JSON data + C++ hooks.
- D-011: staleness.
- D-012: backend choice (Supabase + own relay مقترح).
- D-013: native/web netplay interop.
- D-014: تأجيل Ranked إلى ما بعد الإطلاق.
- D-015: GitHub Actions CI.
- D-016: staged release scope.
- D-017: audio runtime decision.
- D-020: Git LFS.

أي تغيير في هذه القرارات يجب أن يحدّث `27_DECISION_LOG.md` و`00_SHARED_CONSTANTS.md` والاختبارات المتأثرة.

---

## 8. ما الذي نبدأ به الآن؟

**أول مهمة هي P0، وليس بناء نظام جديد مباشرة.**

قائمة العمل التالية:

- [ ] إنشاء clean working clone من `dev/rebirth-continue` الموجود في Git bundle.
- [ ] التحقق من أن `2eeceae` يحتوي على P17 و13-action input fix وكل تعديلات combat المقصودة.
- [ ] مقارنة فرع GitHub master والـrecovery branch والـProduction Bible bundle دون فقدان أي commit.
- [ ] نشر فرع استعادة موثوق على GitHub بعد التحقق، ثم push أول checkpoint.
- [ ] تشغيل Linux Release build و14 CTest suites من clean clone وتسجيل النتائج الجديدة.
- [ ] تحديث Baseline Audit بالحالة الحقيقية لكل نظام.
- [ ] اعتماد نطاق V1.0 وقرارات P1، خصوصاً art pipeline وGamepad scope.

بعد إغلاق P0 ننتقل إلى P1 ثم P2. لا نبدأ بـGamepad implementation قبل تثبيت `InputFrame` و4-player simulation contract، لكن يمكن تصميم صفحة Controller Setup وواجهتها أثناء P1 بالتوازي.

---

## 9. المراجع الأصلية داخل حزمة الاستعادة

هذه الخطة مبنية على وثائق Production Bible الموجودة في `prod/rebirth/`، وبالأخص:

- `00_BASELINE_AUDIT.md`
- `00_PLAN_REVIEW_CORRECTIONS.md`
- `00_SHARED_CONSTANTS.md`
- `01_PRODUCT_VISION.md`
- `02_GAME_DESIGN_MASTER.md`
- `03_COMBAT_SYSTEM.md`
- `04_PHYSICS_AND_MOVEMENT.md`
- `05_FIGHTER_ROSTER.md` وملفات `fighters/`
- `07_ARENA_DESIGN.md`
- `08_ART_DIRECTION.md`
- `09_ART_AND_ASSET_PIPELINE.md`
- `10_ANIMATION_AND_VFX.md`
- `11_AUDIO_AND_MUSIC.md`
- `12_AI_DESIGN.md`
- `13_UI_UX_DESIGN.md`
- `14_INPUT_AND_ACCESSIBILITY.md`
- `15_LOCAL_MULTIPLAYER.md`
- `16_ONLINE_NETCODE.md` و`17_BACKEND_AND_SECURITY.md`
- `18_TECHNICAL_ARCHITECTURE.md` حتى `29_POST_LAUNCH_AND_LIVE_OPERATIONS.md`
- `work-packages/P0` حتى `work-packages/P14`

مرجع SDL3 Gamepad API: https://wiki.libsdl.org/SDL3/SDL_GetGamepads وhttps://wiki.libsdl.org/SDL3/SDL_OpenGamepad وhttps://github.com/libsdl-org/SDL/blob/main/include/SDL3/SDL_gamepad.h

---

**قاعدة الإنهاء:** اللعبة لا تعتبر مكتملة لأن جميع المراحل موجودة في هذه الوثيقة. تعتبر مكتملة عندما تصبح معايير القبول لكل مرحلة خضراء، وتوجد أدلة build/test/playtest، وتكون نسخة الإصدار قابلة للتشغيل فعلياً على المنصات والأجهزة التي نعلن دعمها.
