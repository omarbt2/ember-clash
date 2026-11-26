# EMBER CLASH REBIRTH — الخطة التنفيذية الشاملة حسب الحالة الحالية

| Field | Value |
|---|---|
| Purpose | Current evidence-based execution roadmap for Ember Clash Rebirth, P0–P14. |
| Status | ACTIVE WORK PLAN — phases remain subject to their acceptance gates. |
| Revision date | 2026-11-26 (v2.1) |
| Repository | `omarbt2/ember-clash` |
| Local branch at audit | `reconstruction/rebirth-input-recovery` |
| Code state audited | `1e3175a` |
| Technology | C++20 + SDL3 + CMake; original 2D platform fighter |


> هذه الوثيقة هي نقطة الرجوع التنفيذية الحالية. تحتفظ بخطة P0–P14 وبوثائق التصميم وWork Packages الموجودة في `docs/rebirth/`. لا تستبدلها ولا تبدأ المشروع من جديد. إذا اختلفت وثيقة قديمة مع هذه الخطة في ترتيب التنفيذ أو حالة الإنجاز، فحالة الكود والدليل الحالي هما المرجع، وهذه الوثيقة تحدد ترتيب العمل التالي. قرارات التصميم التفصيلية تبقى في ملفاتها الأصلية وDecision Log.

---

## 1. الهدف النهائي وما نعتبره إصداراً ناجحاً

### 1.1 هدف V1.0 — إصدار محلي متكامل قابل للعب

الإصدار الأول الذي نعتبره جاهزاً للنشر يجب أن يحتوي على:

- قتال منصات 2D سريع ومقروء، بحركة واستجابة وضربات ودفاعات متماسكة، وليس مجرد تشغيل نافذة أو تحريك مربعات.
- **4 مقاتلين كاملين** بميكانيكيات وهوية وحركات ومؤثرات وصوتيات مختلفة فعلاً.
- **3 ساحات مصممة ومكتملة**، مع حدود سقوط وكاميرا وتصادمات ومخاطر إن وجدت، لا layouts إجرائية فقط.
- **اللعب المحلي حتى 4 لاعبين**: أربع وحدات تحكم مستقلة، أو مزيج Keyboard/Gamepad، مع Join/Leave واختيار المقاتل والفريق/الجهة.
- Versus، Free-for-All، Team 2v2، Versus AI، Arcade، Training، Pause/Rematch/Results.
- AI بدرجات Easy/Normal/Hard/Expert، قابل للقياس ولا يعتمد على غشّ المعلومات أو مدخلات اللاعب.
- قائمة رئيسية وإعدادات وتحكم وصوت ورسوم وHUD ونتائج وحفظ محلي؛ كل زر يؤدي وظيفة حقيقية.
- أصول نهائية أصلية مرخصة: رسوم، تحريك، VFX، SFX وموسيقى. يمنع استخدام placeholder أو أصول مجهولة الترخيص في إصدار النشر.
- بناء Release قابل للتكرار على Windows وLinux، ونسخة WebAssembly تجريبية إذا أثبتت توافقها.
- لا crash أو soft-lock معروف، ولا أخطاء تحكم/تصادم/حفظ تمنع اللعب، مع توثيق حدود الأداء على جهاز مرجعي.

### 1.2 ما بعد V1.0

- إكمال roster إلى 8 مقاتلين.
- إكمال 6 ساحات.
- Online 1v1 بعد إغلاق determinism/snapshot gates.
- Ranked/Competitive بعد استقرار online والتحقق من النتائج ومقاومة التلاعب.

Online وRanked ليسا شرطاً لإطلاق V1.0 المحلي، لكنهما شرطان لإعلان اكتمال الرؤية الطويلة Rebirth.

---

## 2. قواعد التنفيذ غير القابلة للتفاوض

1. **الاستمرار من الموجود:** لا إعادة كتابة المشروع من الصفر، ولا تبديل SDL/C++ دون قرار موثق يثبت الضرورة.
2. **مصدر واحد واضح:** لا نسخ ملفات عشوائياً بين bundles/worktrees. أي استعادة أو cherry-pick تحتاج ancestry وdiff واختبارات.
3. **المرحلة لا تساوي ملفاً أو commit:** كل gate يحتاج نتائج اختبار قابلة لإعادة التشغيل ودليل acceptance.
4. **لا ادعاء بناء كامل:** isolated tests أو shim tests لا تعادل SDL integration أو Release build أو تجربة hardware.
5. **لا placeholders في V1:** أي sprite/audio/menu option مؤقت يجب أن يبقى موسوماً ويُزال قبل release gate.
6. **كل تغيير له checkpoint:** فرع عمل، commits صغيرة، لا force-push، لا تعديل master مباشرة.
7. **اختبار قبل وبعد:** أعد تشغيل الاختبارات ذات الصلة ثم suite المشروع؛ إذا فشل build بسبب network/toolchain فوثّق BLOCKED ولا تعتبره PASS.
8. **حافظ على المشروع قابلاً للتشغيل:** لا تفتح نطاقاً جديداً إذا كان هناك blocker أساسي في compile/runtime أو input/physics.
9. **القرارات المهمة تسجل:** أي تغيير في scope أو constants أو schema أو controls أو networking يضاف إلى `27_DECISION_LOG.md`.
10. **الأصول حقوقها واضحة:** كل ملف إنتاجي له source/creator/license/attribution/usage record في inventory.
11. **لا نطارد عدد الميزات:** الأولوية لعمق وجودة القتال المحلي، ثم roster/stages، ثم online.
12. **العمل عبر plugins لا يعوض التحقق:** GitHub connector للنشر/الفرع/الملفات؛ أدوات التصميم/الأصول للمساعدة في الإنتاج، لكنها لا تثبت دمج asset أو صلاحية runtime.

### حالات التحقق الرسمية

- `VERIFIED`: اختبار متكامل مناسب نجح، مع دليل وبيئة محددة.
- `PARTIALLY VERIFIED`: أجزاء نجحت لكن gate الكامل أو اختبار الجهاز لم يكتمل.
- `UNVERIFIED`: لا يوجد دليل كافٍ أو لم يُشغّل.
- `MISSING`: الميزة/المحتوى غير موجود.
- `BLOCKED`: لا يمكن مواصلة التحقق بسبب blocker محدد (مثل تعذر تنزيل SDL).
- `REGRESSED`: كان يعمل ثم فشل بعد تغيير.

---

## 3. لقطة الحالة الحالية — 26 نوفمبر 2026

هذه الحالة مبنية على الفرع المحلي المستعاد وملف evidence `docs/rebirth/evidence/2026-11-26-full-plan-status.md`، مع تحديث HEAD إلى `1e3175a`.

| المجال | الموجود أو المثبت حالياً | الحالة | الفجوة/الشرط التالي |
|---|---|---|---|
| Recovery/Git | فرع استعادة محلي؛ تاريخ محفوظ؛ bundle وzip قابلان للتحقق؛ شجرة نظيفة عند checkpoint | PARTIALLY VERIFIED | مصالحة remote history وإعادة نشر فرع المصدر دون force-push؛ clean clone baseline |
| Docs/Design | Production Bible، shared constants، design docs، fighter briefs، work packages | PARTIALLY VERIFIED | إغلاق قرارات OPEN/PROPOSED وتحديث decision log وفق التنفيذ |
| Build/CI | Linux/Windows CI definitions؛ WebAssembly job مضاف؛ YAML وفحوص docs نجحت | BLOCKED / PARTIAL | تشغيل CI فعلياً؛ SDL FetchContent تعطل DNS؛ full build وCTest الحاليان لم ينجحا/لم يُشغلا |
| Physics | shared fixed-step 60Hz في Desktop/Web، input edge latches، dt-free APIs، timers integer ticks، four-sided blast zones، 16px collision substeps؛ 25 test suites تمر عبر GCC/Clang وGCC ASan+UBSan | PARTIALLY VERIFIED | controlled replay أصبح يحقق tolerance/event gate بعد إصلاح respawn timer (20 scripts / 37,200 rows، 0 event mismatch، max position delta 0.000122 px)؛ side blast-zone delta مقصود وموثق؛ D-001 ما زال PROPOSED بانتظار sign-off، مع D-002، POD snapshot، ledge/DI/tech، full CMake/CTest وmanual feel test مفتوحة |
| Combat | attack table validation وguard/parry/dodge/Surge foundations | PARTIALLY VERIFIED | combat resolution متكامل، move data، hitstop/priority/clank/staleness/grab/projectiles، matchup tests |
| Fighters | roster design لثمانية؛ Match ما زال محدوداً بمقاتلين؛ art procedural | PARTIAL / MISSING | مقاتلان أولاً بعمق كامل ثم 4 لـV1، assets و47 clips حسب production bible لكل شخصية |
| AI/Training | presets حتى Expert، recorder محدود 600 tick، أساس playback/actions | PARTIALLY VERIFIED | behavior quality، fairness metrics، recording/playback UI، bots ضد 2–4 fighters |
| Arenas | 3 layouts بيانات/تصادم واختبارات isolated | PARTIALLY VERIFIED | authored visual art، camera/hazards، audio/ambience، stage QA؛ 6 لاحقاً |
| Gamepad/4P | event pump ownership/hotplug snapshot foundation؛ key probe عبر shim | PARTIALLY VERIFIED | SDL virtual/hardware integration، device registry، four independent slots، remap/calibration |
| UI/Modes | menu/settings/input foundations وبعض mode/stage state | PARTIALLY VERIFIED | complete player join/select/stage/settings/results/training/profile flows؛ كل controls تعمل |
| Audio | procedural audio foundation/tests | PARTIALLY VERIFIED | authored sound/music، mixer/asset loading، سماع فعلي ومكساج |
| Online | state hash/fingerprint foundation فقط | MISSING | deterministic snapshots/input history ثم rollback/transport/rooms/reconnect |
| Ranked | لا pipeline موثوق للنتائج أو rating | MISSING | server validation/rating/seasons/abuse checks بعد online |
| Release | docs 60 files + 7 asset validator tests + عدة isolated C++ tests PASS | PARTIAL | clean full Release builds، platforms، performance, packaging, playtests |

### القيود الحالية الصريحة

- CMake Release configure توقف عند SDL3 FetchContent لأن DNS لم يستطع حل `github.com`؛ تنزيل SDL من `libsdl.org` تعطل كذلك. لذلك full CMake build وCTest لم يُثبت نجاحهما في هذا checkpoint.
- اختبارات InputManager مع shim ليست اختبار SDL3 أو أجهزة حقيقية.
- CI WebAssembly workflow أُضيف، لكن لم يُثبت أنه نجح على GitHub بعد.
- push للفرع المحلي لم ينجح في المحاولة السابقة بسبب DNS. لا نكتب `PUSHED` حتى يظهر commit/branch على GitHub فعلياً.
- لا توجد أصول إنتاجية نهائية أو 4P فعلي أو Online/Ranked مكتملة حتى الآن.

---

## 4. ترتيب التنفيذ المعتمد — لا ننفذ P0–P14 كقائمة عمياء

الترتيب الأصلي محفوظ، لكن التنفيذ العملي يعتمد على gates والاعتماديات. لا نبدأ art mass-production أو UI polishing قبل استقرار combat/movement contracts.

### Track A — إعادة تثبيت البناء والقرارات (P0 → P1 → Build Gate)

**A0 — Reproducible baseline**
- حل/تجاوز blocker SDL بشكل موثق: شبكة متاحة، cache/source archive موثوق مع hash، أو build runner متصل.
- clean checkout من الفرع المعتمد، CMake configure/build Release، CTest كامل.
- تسجيل compiler/SDL/CMake versions، commit SHA، test count، warnings، artifact path.
- التحقق من Windows وWASM CI jobs، وعدم اعتبار YAML parse نجاحاً للبناء.

**A1 — P0 source/branch reconciliation**
- مقارنة source bundle وremote branch وmaster؛ تحديد المصدر المعتمد.
- حفظ history وعدم force-push؛ فتح branch واضح لنشر المصدر المحلي إذا لزم.
- مراجعة `P0_Recovery_and_Audit.md` وevidence وتحديث نقطة البداية.

**A2 — P1 decision closure**
- إغلاق قرارات D-001…D-020 حسب Decision Log، خصوصاً fixed tick/determinism، snapshot completeness، input semantics، fighter data، asset pipeline، online scope.
- مراجعة shared constants وعدم تكرار القيم في gameplay/UI.
- كل قرار: context، selected option، tradeoff، affected files، test needed.

**Gate A:** clean baseline build/test، source lineage موثق، القرارات التي تمنع P2 مغلقة. إذا SDL لا يزال blocked، نكمل isolated code فقط ولا نعلن integration green.

### Track B — أساس القتال deterministic (P2 → P3)

**B1 — P2 Physics & Movement**
1. تسجيل baseline replay/reference قبل تغيير الزمن.
2. إكمال fixed timestep وsimulation clock؛ فصل render frame عن simulation tick.
3. نقل المؤقتات gameplay إلى tick/frame-based representation حيث يلزم؛ لا خلط wall-clock مع simulation.
4. استكمال jump buffer/coyote/double jump/run/dash/drop-through/ledge/air control/DI/recovery.
5. تعريف blast zones، knockback، friction، gravity، fast-fall، landing lag، hitstun، input buffering.
6. إضافة deterministic replay test، seed control، state serialization/hash، regression vectors.
7. collision tests: أرض/منصة/حافة/تصادم fighters/إعادة دخول/سقوط؛ اختبار frame boundaries.
8. لا تضبط الإحساس بالقتال عبر constants عشوائية؛ tuning عبر ملف/جداول مع test وplaytest notes.

**B2 — P3 Combat Engine**
1. فصل move data عن code paths قدر الإمكان؛ schema validation لكل attack.
2. hitbox/hurtbox lifecycle وactive frames، startup/recovery، hitlag/hitstop.
3. damage accumulation وknockback scaling/angle، DI/SDI، launch decay إذا معتمد.
4. guard/shield health/regen/break، parry/dodge windows، cooldowns.
5. priority/clank/trade، armor، grabs/throws، projectile/trap لو ضمن design.
6. staleness/queue rules، invulnerability، edge cases وsimultaneous hits.
7. test matrix لكل نوع هجمة ودفاع، وscenario tests في Match وليس data-only فقط.
8. replay/hash consistency بعد كل scenario.

**Gate B:** deterministic replay suite ثابتة؛ جميع combat cases المعتمدة PASS؛ لا regression في movement/attack timing؛ snapshot/state hash يغطي كل state مؤثر.

### Track C — Vertical Slice مقاتلين اثنين بجودة حقيقية (P4 + P5 بالتوازي بعد B)

**C1 — P4 Two Fully Playable Fighters**
- اختيار Kaelen وTorga كـ first vertical slice حسب design docs، ما لم Decision Log يثبت اختياراً آخر.
- لكل شخصية: move list كاملة، passive/unique hook، balance identity، attacks/defenses/recovery، AI support، data validation.
- لا نضيف roster كبير قبل اختبار الشخصيتين ضد بعضهما وضد AI.
- Acceptance: matchup matrix، move coverage، no invalid move data، gameplay recording، external playtest feedback.

**C2 — P5 Art/Animation Pipeline (مسار إنتاج منفصل وموازٍ)**
1. اعتماد style bible، silhouette/scale/palette، animation naming، frame rate، pivot/anchor، atlas format.
2. تجهيز source art وpipeline من source file إلى exported asset، مع deterministic validation.
3. لكل مقاتل clips المطلوبة من production bible (هدف 47 clip/character؛ راجع القائمة الأصلية، لا تخترع clips بديلة).
4. إنشاء/استيراد idle/run/jump/fall/land/dodge/guard/parry/hitstun/KO/attacks/intro/victory، مع frames/variants حسب move.
5. إضافة VFX وimpact readability، hurt/hit indicators، UI icons/portrait.
6. asset validator: dimensions, frame count, naming, missing frames, license metadata, file sizes.
7. runtime loader + fallback واضح أثناء التطوير فقط؛ release fails إذا asset required missing.
8. اختبار readability عند 720p و1080p، وتباين ألوان، silhouettes، animation transitions.

**Gate C:** شخصيتان playable end-to-end مع art/audio pass معتمد؛ لا procedural placeholder في release path؛ 47 clip checklist لكل شخصية مكتمل أو استثناء رسمي موثق.

### Track D — AI/Training/Screens/Local multiplayer (P6 → P7 → P8 → P9)

**D1 — P6 AI & Training**
- Easy/Normal/Hard/Expert profiles بparameters معلنة؛ reaction delay واقعي؛ لا قراءة state مخفي غير مسموح.
- behavior states: approach, spacing, punish, defend, recover, edgeguard حسب design؛ لا AI يكرر نفس move بصورة مملة.
- metrics: action diversity، punish rate، win-rate band، response fairness، difficulty consistency.
- Training: reset position/damage/stocks، frame/action display، dummy behaviors، record/playback، hitbox view، inputs history.
- 600-tick recorder يبقى bounded؛ اختبارات memory cap، overflow، pause/rewind policy.

**D2 — P7 Arenas (3 V1 arenas)**
- تحويل layouts الإجرائية إلى stage data/art كاملة؛ لكل arena background layers، platforms، blast zones، camera bounds، hazards إن وجدت.
- stage-specific collision and hazard tests؛ audio ambience؛ performance and visual QA.
- تصميم المنافسة أولاً؛ hazards اختيارية ومضبوطة وليست random unfair.

**D3 — P8 Gamepad & Local Multiplayer 4P**
1. SDL event pump واحد؛ dispatch events إلى input/gamepad/window؛ no double-poll.
2. Gamepad registry متعددة الأجهزة، stable device IDs، hotplug/removed handling، rumble capability.
3. Player slot manager لـP1–P4؛ device assignment وkeyboard sharing policy؛ join/leave.
4. Input profile لكل لاعب، rebinding، deadzone/calibration، duplicate binding rules، reset defaults.
5. test virtual SDL gamepads + physical controller matrix (Xbox/PlayStation/generic حسب المتاح).
6. 4P Match update/render/camera/UI/score/stocks؛ collision load وframe-time.
7. test disconnect/reconnect، controller order changes، unplug أثناء menu/match، duplicate controllers.

**D4 — P9 Modes & Complete UI**
- Flow: Boot → Main Menu → Mode → Player Join → Fighter Select → Team/Rules → Stage Select → Match → Pause → Results → Rematch/Return.
- Versus/FFA/2v2/AI/Arcade/Training فعالة، وكل option يطبق settings فعلياً.
- controller navigation + focus states + accessibility (remap, text clarity, color-safe signals).
- HUD: damage/knockback feedback، stocks/time، team identity، player colors، pause/results.
- settings persist safely؛ corrupt/missing config recovery؛ no menu soft-lock.
- smoke-test لكل flow، وكل زر/setting مرتبط باختبار أو manual checklist.

**Gate D:** 4P local match حقيقي على أجهزة input منفصلة؛ 3 arenas؛ 4 fighters مستهدفين للـV1؛ UI كاملة مع جميع modes المعتمدة؛ AI/training playable.

### Track E — توسيع V1، Release، ثم post-launch (P10 + P14 → P11 → P12 → P13)

**E1 — P10 Eight-Fighter Roster**
- V1 release: إكمال 4 fighters أولاً (Kaelen/Torga/Vesper/Isolde حسب roster bible).
- post-launch wave: Rook/Maren/Zephyr/Bastian؛ لا نعتبر التصميم النصي شخصية playable.
- لكل شخصية: gameplay identity، 47 clips، audio/VFX، AI، matchup/balance tests، asset/license manifest.

**E2 — P14 Release Preparation (gate متكرر، وليس آخر خطوة فقط)**
- CI clean builds Linux/Windows/WASM، unit/integration/replay tests.
- Release packaging، versioning، crash logs، save/config migration، installer/portable policy.
- performance budgets: 60 FPS target على جهاز baseline محدد، memory/asset loading، 4P stress.
- 1000-match fuzz/simulation (حسب test harness)؛ crash/soft-lock scan؛ known issues list.
- credits/licenses، store screenshots/trailer/description، controller support disclosure.
- external playtests، feedback triage، release candidate، rollback plan.

**E3 — P11 Arenas 4–6**
- ثلاثة arenas إضافية؛ art/collision/camera/hazards/music/QA؛ لا copy-paste theme/layout.

**E4 — P12 Online 1v1**
- لا يبدأ production netcode قبل Gate B determinism + complete snapshots.
- architecture review: rollback/input delay tradeoffs، transport، matchmaking/rooms، relay/NAT، reconnect، version compatibility.
- offline simulation: packet loss/jitter/latency، rollback window، desync detection/recovery.
- security: input validation، rate limits، no trust in client result، telemetry privacy.
- real two-device test؛ closed beta؛ no ranked during initial online stabilization.

**E5 — P13 Ranked/Competitive**
- authoritative/verified result path؛ anti-cheat limits clearly stated؛ rating (e.g. Elo/Glicko selected in decision log)، seasons/leaderboards، disconnect policy.
- abuse/reporting/moderation/privacy/retention policies.
- server-side replay/result verification، matchmaking integrity، exploit regression suite.
- ranked launch only after online stability and competitive playtest gate.

---

## 5. قائمة التنفيذ القريبة — ما نبدأ به فعلياً

هذه هي قائمة العمل التالية بالترتيب، وليست اقتراحات عامة:

### Sprint 0 — Build & Source Recovery
- [ ] تحقق من HEAD/branch/tree والـbundle قبل أي تعديل.
- [ ] جرب build runner/CI connected؛ حل SDL source dependency بدون تبديل نسخة SDL بصمت.
- [ ] أعد clean Release configure/build وCTest وسجل output.
- [ ] تحقق من GitHub branch ancestry وقم بنشر source branch آمن إذا سمحت الشبكة.
- [ ] حدّث P0 evidence ببيانات build/push الحقيقية.

### Sprint 1 — Decision & Determinism Gate
- [ ] راجع D-001…D-020 وصنّف OPEN/PROPOSED/DECIDED.
- [ ] أغلق القرارات التي تؤثر في physics, replay, state snapshot, inputs.
- [ ] أضف deterministic replay regression test end-to-end.
- [ ] راجع state hash coverage وtimer conversion؛ أضف test يكشف state غير محفوظ.
- [ ] نفذ P2 movement edge-case matrix مع integration في Match.

### Sprint 2 — Combat Completeness
- [ ] اكمل move data schema + validation.
- [ ] اكتب combat scenario tests: hit, trade, shield, parry, dodge, KO/ring-out, multi-hit.
- [ ] اكمل priority/clank/staleness/grab/throw حسب design scope.
- [ ] تحقق من fighter state transitions بعد hitstun/knockback/landing/recovery.
- [ ] احفظ replay لكل bug combat وأضفه regression fixture.

### Sprint 3 — First Fighter Vertical Slice + Asset Pipeline
- [ ] احسم أول مقاتلين وmove list النهائيين من fighter briefs.
- [ ] ثبت art export format، pivot/anchors، frame naming، manifest/licensing.
- [ ] اربط runtime asset loader بالأصول بدلاً من procedural art.
- [ ] أتمم جميع clips والمؤثرات والصوتيات للشخصيتين.
- [ ] نفذ matchup QA وتسجيل gameplay قابل للمراجعة.

### Sprint 4 — Local 4P & Complete Menus
- [ ] اكتب virtual controller integration tests عند توفر SDL3.
- [ ] نفذ multi-device registry وplayer slot manager.
- [ ] نفذ join/select/stage/rules/results flow.
- [ ] اختبر 4P frame budget، disconnect/reconnect، keyboard+gamepad.
- [ ] اختبر جميع UI actions وnavigation من keyboard/controller.

### Sprint 5 — V1 Content & Release Candidate
- [ ] أكمل 4 fighters و3 arenas وAI/training/modes.
- [ ] أتمم authored audio/music/VFX/UI assets وlicense audit.
- [ ] clean release build على Windows/Linux؛ WebAssembly عند ثبوت scope.
- [ ] performance + crash + 1000-match fuzz + external playtest.
- [ ] release candidate وknown issues/rollback/package docs.

ثم P10 wave الثانية، P11 arenas، P12 online، P13 ranked حسب gates أعلاه.

---

## 6. طريقة إدارة كل Work Package

لكل task/phase، نستخدم بطاقة التنفيذ التالية في issue أو commit description:

- **Scope:** ما الذي سيتغير، وما الذي لن يتغير.
- **Prerequisites:** commits/decisions/tests التي يجب أن تكون جاهزة.
- **Implementation:** الملفات والنظام المتوقع تعديله.
- **Tests:** unit + integration + replay/manual hardware حسب النوع.
- **Acceptance:** شروط قابلة للقياس؛ لا عبارات مثل «أصبح أفضل» فقط.
- **Evidence:** command output، screenshot/video عند الحاجة، test report، commit SHA.
- **Risk/rollback:** كيف نرجع إذا فشل التغيير.

### شروط إغلاق المرحلة

لا تغلق المرحلة إلا إذا:
1. جميع acceptance criteria في work package الأصلية مكتملة أو استثناءاتها موافق عليها ومكتوبة.
2. tests نجحت على البيئة المطلوبة؛ أي blocked موثق ولا يُخفى.
3. build/test suite المناسب نجح بعد آخر تعديل.
4. manual QA اكتمل عندما يكون hardware/visual/audio/gameplay ضرورياً.
5. evidence file محدّث، وcommit checkpoint موجود.
6. لا يوجد placeholder أو known blocker داخل scope المرحلة.

---

## 7. Git، artifacts، وGitHub workflow

- فرع العمل الأساسي يبقى `reconstruction/rebirth-input-recovery` محلياً حتى تتم مصالحة remote ancestry.
- لا نرفع التاريخ المحلي إلى فرع remote مختلف دون فحص؛ لا force-push مطلقاً.
- كل phase checkpoint: `rebirth-Pn-<short-description>` بعد قبول gate، وليس قبلها.
- Commit messages واضحة ومحددة؛ commits صغيرة حسب subsystem.
- قبل push: `git status` clean، docs/tests/build المناسب، `git log --graph`، branch comparison.
- بعد push: تحقق من remote SHA وbranch فعلياً عبر GitHub plugin/API.
- عند تعذر الشبكة: احفظ local commit + bundle/patch/zip موثقة، واذكر `PUSH BLOCKED`.
- لا تعتبر إنشاء ملف عبر GitHub API على branch منفصل أنه نشر كامل source branch.

### Plugins/tooling

1. **GitHub plugin**: مراجعة repo/branches/commits، إنشاء فرع/ملف عند الحاجة، والتحقق من push/PR/Actions. لا يثبت build محلياً وحده.
2. **Creative/visual asset tools** (مثل Higgsfield أو image workflow): concept art، exploration، reference boards، أو أصول موجهة. أي output يحتاج art direction review، format/export integration، licensing/usage record، وin-game QA قبل اعتماده.
3. **Local build/test tooling**: CMake/Ninja/CTest/compiler/SDL3/Emscripten؛ هذا المسار الحاسم لإثبات الكود.
4. لا نحتاج plugin إضافياً الآن لمجرد كتابة الخطة؛ نستخدم الموجود، ونبحث عن أداة أخرى فقط إذا ظهر احتياج واضح (مثل CI runner أو asset pipeline).

---

## 8. Dashboard الحالة

يُحدّث هذا الجدول مع كل checkpoint، ولا نستخدم كلمة DONE دون evidence:

| Phase | الحالة الحالية | التالي | دليل الإغلاق |
|---|---|---|---|
| P0 Recovery | PARTIALLY VERIFIED | source/remote reconciliation + clean baseline | P0 evidence + clean build/test |
| P1 Design/Technical | PARTIALLY VERIFIED | decision log closure | decisions closed + shared constants |
| P2 Physics | PARTIALLY VERIFIED | determinism/replay + integrated movement | replay suite + physics matrix |
| P3 Combat | PARTIALLY VERIFIED | scenario completeness + snapshot coverage | combat matrix + replay regressions |
| P4 Two fighters | PARTIAL | full playable vertical slice | two complete fighters + matchup QA |
| P5 Art pipeline | PARTIAL/MISSING production assets | pipeline + full clip set | validated authored assets + runtime QA |
| P6 AI/Training | PARTIALLY VERIFIED | fairness/quality metrics + full training UI | benchmark + manual training QA |
| P7 Three arenas | PARTIALLY VERIFIED | authored visuals/audio/camera | 3 production arenas + stage QA |
| P8 Local 4P | PARTIALLY VERIFIED | multi-device 4-slot integration | 4P hardware/virtual test matrix |
| P9 UI/Modes | PARTIALLY VERIFIED | complete flows + all buttons | end-to-end mode/navigation suite |
| P10 Eight fighters | MISSING full roster | V1 four complete, then wave 2 | 8 complete fighters + balance QA |
| P11 Six arenas | MISSING full set | 3 additional arenas | 6 production arenas + QA |
| P12 Online | MISSING | start only after deterministic gate | two-device net test + resilience suite |
| P13 Ranked | MISSING | after online stabilization | validated ranking + security/abuse QA |
| P14 Release | PARTIALLY VERIFIED/BLOCKED | reproducible build + release candidate | platform matrix + playtest + package |

---

## 9. Definition of Done للمشروع الكامل

يُعلن **V1.0 Local Release** فقط عند اكتمال P0–P9 بالـscope المحدد لـV1، وإكمال P14 release gates، مع 4 fighters و3 arenas و4P فعلي وbuilds موثقة.

يُعلن **Full Rebirth Roadmap Complete** فقط عند اكتمال P10–P13 أيضاً: 8 fighters، 6 arenas، Online 1v1 مستقرة، Ranked/Competitive متحقق منها، مع P14 release/operations evidence.

إذا بقي أي gate blocked أو unverified، نذكره بوضوح ولا نغطيه بكلمة «مكتمل».

---

## 10. تعليمات الاستمرار من هذه الوثيقة (Agent Handoff)

عند استئناف العمل في جلسة جديدة أو بواسطة أداة/agent آخر، يجب اتباع هذا التسلسل حرفياً:

1. افتح هذه الوثيقة، ثم `docs/rebirth/evidence/2026-11-26-full-plan-status.md`، ثم Work Package للمرحلة التالية، ثم الملفات المصدرية المرتبطة بها.
2. افحص `git status` و`git log` والفرع والـremote قبل التعديل؛ لا تفترض أن الملفات أو history في remote مطابقة للـworkspace المحلي.
3. ابدأ بأول gate غير مغلق في Sprint 0/Sprint 1؛ لا تقفز إلى إضافة محتوى جديد إذا كان build أو source lineage غير مستقر.
4. نفذ تغييراً صغيراً قابلاً للمراجعة، ثم اختبره. إذا تعذر اختبار التكامل، أضف اختباراً مستقلاً مفيداً وسجل حدود دليله، ولا تسمّه integration test.
5. لا تتوقف عند أول blocker: افحص cache/toolchain/runner أو نفذ الجزء المستقل الآمن، ثم سجل ما بقي blocked وسبب ذلك. لا تغيّر dependency أو architecture بصمت لتجاوز blocker.
6. بعد كل مجموعة تغييرات: شغّل الاختبارات المناسبة، `git diff --check`، docs checker عند تعديل الوثائق، حدّث evidence/status، ثم commit محلياً.
7. لا تنشئ phase-done tag، ولا تكتب BUILD PASS أو PUSHED أو APK/Release READY إلا بعد تحقق مباشر من gate المقصود.
8. قبل أي push: قارن ancestry مع GitHub؛ استخدم branch جديداً إذا كان التاريخ مختلفاً، ولا force-push ولا تعدّل `master` مباشرة. تحقق بعد النشر من SHA الموجود على GitHub.
9. عند تسليم العمل، اذكر: ما تغير، commit، الاختبارات التي نجحت، التي لم تُشغّل، blockers، والملف/الحزمة التي يمكن للمستخدم استكمالها.

**Prompt مختصر للاستئناف:**

> أكمل Ember Clash Rebirth من المستودع الحالي، ولا تبدأ من الصفر. اقرأ `docs/rebirth/MASTER_EXECUTION_ROADMAP_2026-11-26_AR.md` وملف evidence وWork Package التالية قبل التعديل. افحص Git lineage والـworking tree. نفّذ أول gate غير مكتمل حسب الخطة، مع أقصى عمل آمن ممكن، واختبارات حقيقية وcommits صغيرة. لا تدّعِ نجاح build أو push أو اكتمال phase دون دليل. حدّث evidence والـroadmap بعد العمل، وصدّر bundle/patch إذا تعذر push.

---

## 11. سجل التحديثات

- **2026-11-26 — v2.0:** إعادة ترتيب خطة P0–P14 حسب evidence الحالي، تحديد V1 مقابل post-launch، إضافة execution sprints، gates، current build/network limitations، workflow GitHub/plugins، وDefinition of Done. الحالة مستمدة من `docs/rebirth/evidence/2026-11-26-full-plan-status.md` وHEAD `1e3175a`.
- **2026-11-26 — v2.1:** إضافة تعليمات الاستئناف العملية وprompt handoff؛ توضيح الفرق بين commit الكود الذي دُققت حالته (`1e3175a`) وcommit الوثيقة (`d04b8a7`).
