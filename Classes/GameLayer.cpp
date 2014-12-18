//
//  GameLayer.cpp
//  PuzzleGame
//
//  Created by NAKASHIMAshinichiro on 2014/11/18.
//
//

#include "GameLayer.h"

#define BALL_NUM_X 6
#define BALL_NUM_Y 5
#define WINSIZE Director::getInstance()->getWinSize()
#define TAG_LEVEL_LAYER 10000

USING_NS_CC;

// コンストラクタ
GameLayer::GameLayer()
: _movingBall(nullptr)
, _movedBall(false)
, _touchable(true)
, _maxRemovedNo(0)
, _chainNumber(0)
, _level(0)
, _nextlevel(0) {
    // 乱数初期化および各ボールの出現の重みを指定
    std::random_device device;
    _engine = std::default_random_engine(device());     // ボールの重みを設定
    _distForBall = std::discrete_distribution<int> {20, 20, 20, 20, 20, 10};
    _distForMember = std::uniform_int_distribution<int> (0, 4);                // 0、1、2、3、4のいずれかの値を取得することができる
}

// シーンの生成
Scene* GameLayer::createScene(int level) {
    auto scene = Scene::create();
    auto layer = GameLayer::create(level);
    scene->addChild(layer);
    
    return scene;
}

/** インスタンス生成 */
GameLayer* GameLayer::create(int level) {
    GameLayer *pRet = new GameLayer();
    pRet->init(level);
    pRet->autorelease();
    
    return pRet;
}

// 初期化
bool GameLayer::init(int level) {
    if (!Layer::init()) {
        return false;
    }
    
    // シングルタップイベントの取得
    auto touchListener = EventListenerTouchOneByOne::create();          // イベントリスナーに対して、シングルタップイベントを関連づける
    touchListener->setSwallowTouches(_swallowsTouches);
    touchListener->onTouchBegan = CC_CALLBACK_2(GameLayer::onTouchBegan, this);
    touchListener->onTouchMoved = CC_CALLBACK_2(GameLayer::onTouchMoved, this);
    touchListener->onTouchEnded = CC_CALLBACK_2(GameLayer::onTouchEnded, this);
    touchListener->onTouchCancelled = CC_CALLBACK_2(GameLayer::onTouchCancelled, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);
    
    _level = level;             // レベルの保持
    
    initBackground();           // 背景の初期化
    initBalls();                // ボールの初期表示
    initEnemy();                // 敵の表示
    initMembers();              // メンバーの表示
    initLevelLayer();           // レベル表示レイヤーの表示
    
    return true;
}

// 背景の初期化
void GameLayer::initBackground() {
    // キャラクター部分の背景
    auto bgForCharacter = Sprite::create("Background1.png");
    bgForCharacter->setAnchorPoint(Point(0, 1));
    bgForCharacter->setPosition(Point(0, WINSIZE.height));
    addChild(bgForCharacter, ZOrder::BgForCharacter);
    
    // パズル部分の背景
    auto bgForPuzzle = Sprite::create("Background2.png");
    bgForPuzzle->setAnchorPoint(Point::ZERO);
    bgForPuzzle->setPosition(Point::ZERO);
    addChild(bgForPuzzle, ZOrder::BgForPuzzle);
}

// ボールの初期化
void GameLayer::initBalls() {
    for (int x = 1; x <= BALL_NUM_X; x++) {
        for (int y = 1; y <= BALL_NUM_Y; y++) {
            // ボールを生成する
            newBalls(BallSprite::PositionIndex(x, y), true);  // ボールの配列分（6列×5行)生成する
        }
    }
}

// 新規ボールの作成
BallSprite* GameLayer::newBalls(BallSprite::PositionIndex positionIndex, bool visible) {
    // 現在のタグを取得
    int currentTag = BallSprite::generateTag(positionIndex);
    
    // 乱数を元に、ランダムでタイプを取得
    int ballType;
    while (true) {
        ballType = _distForBall(_engine);       // ランダムでボールの種類を決める
        
        // 妥当性のチェック（ボールが隣り合わせにならないようにする）
        
        // 左隣のボール（3つ以上同じ種類のボールが並ばないようにする）
        auto ballX1Tag = currentTag - 10;       // 1つ左隣は10引いた値
        auto ballX2Tag = currentTag - 20;       // 2つ左隣は20引いた値
        auto ballX1 = (BallSprite*)(getChildByTag(ballX1Tag));
        auto ballX2 = (BallSprite*)(getChildByTag(ballX2Tag));
        
        // 現在のボールが、1つ左隣と2つ左隣のボールと同じだとNG
        if (!(ballX1 && ballType == (int)ballX1->getBallType()) ||
            !(ballX2 && ballType == (int)ballX2->getBallType())) {
            // 下隣のボール
            auto ballY1Tag = currentTag - 1;        // 1つ下隣は1引いた値
            auto ballY2Tag = currentTag - 2;        // 2つ下隣は2引いた値
            auto ballY1 = (BallSprite*)(getChildByTag(ballY1Tag));
            auto ballY2 = (BallSprite*)(getChildByTag(ballY2Tag));
            
            // 現在のボールが、1つ下隣と2つ下隣のボールと同じだとNG
            if (!(ballY1 && ballType == (int)ballY1->getBallType()) ||
                !(ballY2 && ballType == (int)ballY2->getBallType())) {
                // 左隣と下隣が揃わない場合は、ループを抜ける
                break;
            }
        }
    }
    
    // ボールの表示
    auto ball = BallSprite::create((BallSprite::BallType)ballType, visible);
    ball->setPositionIndexAndChangePosition(positionIndex);
    addChild(ball, ZOrder::Ball);
    
    return ball;
}

bool GameLayer::onTouchBegan(Touch* touch, Event* unused_event) {
    if (!_touchable) {
        return false;
    }
    
    _movedBall = false;
    _movingBall = getTouchBall(touch->getLocation());
    
    if (_movingBall) {
        return true;
    }else{
        return false;       // どのボールもタップされていない場合は、以降のタップイベントを取得しない
    }
}

void GameLayer::onTouchMoved(Touch* touch, Event* unused_event) {
    // スワイプとともにボールを移動する
    _movingBall->setPosition(_movingBall->getPosition() + touch->getDelta());           // 前回の位置との差分を、ボールの位置に加算することで、ボールを移動させる
    
    auto touchBall = getTouchBall(touch->getLocation(), _movingBall->getPositionIndex());
    if (touchBall && _movingBall != touchBall) {
        // 移動しているボールが、別のボールの位置に移動
        _movedBall = true;
        
        // 別のボールの位置インデックスを取得
        auto touchBallPositionIndex = touchBall->getPositionIndex();                    // 新たに触れたボールの位置インデックスを取得する
        
        // 別のボールを移動しているボールの元の位置へ移動する
        touchBall->setPositionIndexAndChangePosition(_movingBall->getPositionIndex());  // 位置インデックスを交換する
        
        // 移動しているボールの情報を変更
        _movingBall->setPositionIndex(touchBallPositionIndex);                          // 位置インデックスを交換
    }
}

void GameLayer::onTouchEnded(Touch* touch, Event* unused_event){
    // タップ操作によるボールの移動完了時の処理
    movedBall();
}

void GameLayer::onTouchCancelled(Touch* touch, Event* unused_event){
    onTouchEnded(touch, unused_event);          // タップ終了イベントと同じ処理を行う
}

// タップした位置のチェック
BallSprite* GameLayer::getTouchBall(Point touchPos, BallSprite::PositionIndex withoutPosIndex) {
    for (int x = 1; x <= BALL_NUM_X; x++) {
        for (int y = 1; y <= BALL_NUM_Y; y++) {
            if (x == withoutPosIndex.x && y == withoutPosIndex.y) {
                // 指定位置のボールの場合は、以下の処理を行わない
                continue;
            }
            
            // タップ位置にあるボールかどうかを判断する
            int tag = BallSprite::generateTag(BallSprite::PositionIndex(x, y));
            auto ball = (BallSprite*)(getChildByTag(tag));
            if (ball) {
                // 2点間の距離を求める
                float distance = ball->getPosition().getDistance(touchPos);
                
                // ボールの当たり判定は円形、つまりボールの中心からの半径で判断する
                if (distance <= BALL_SIZE / 2) {        // BALL_SIZEは直径であるため、「2」で割り半径としている
                    // タップした位置にボールが存在する
                    return ball;
                }
            }
        }
    }
    
    return nullptr;
}

// タップ操作によるボールの移動完了時の処理
void GameLayer::movedBall() {
    // 移動しているボールを本来の位置に戻す
    _movingBall->resetPosition();       // スワイプ処理が終わったため、正しい位置に移動する
    _movingBall = nullptr;
    
    // 一列に並んだボールがあるかチェックする
    _chainNumber = 0;
    _removeNumbers.clear();
    checksLinedBalls();
}

// 一列に並んだボールがあるかチェックする
void GameLayer::checksLinedBalls() {
    // 画面をタップ不可とする
    _touchable = false;                 // アニメーション中は、ボールを移動できないようにする
    
    if (existsLinedBalls()) {
        // 3個以上並んだボールの存在する場合
        
        // 連鎖カウントアップ
        _chainNumber++;
        
        // ボールの削除と生成
        removeAndGenerateBalls();       // ボールのアニメーションを準備する
        
        // アニメーション後に再チェック
        auto delay = DelayTime::create(ONE_ACTION_TIME * (_maxRemovedNo + 1));
        auto func = CallFunc::create(CC_CALLBACK_0(GameLayer::checksLinedBalls, this)); // ボールのアニメーションが終わったら、再度ボールの並びをチェックする
        auto seq = Sequence::create(delay, func, nullptr);
        runAction(seq);
    }else{
        // CalculateDamage関数で計算する値の初期化
        int chainNum = 0;
        int damage = 0;
        int healing = 0;
        std::set<int> attackers;
        
        // ダメージ・回復の計算
        calculateDamage(chainNum, healing, damage, attackers);
        
        // 敵にダメージを与える
        int afterHp = _enemyData->getHp() - damage;
        
        if (damage > 0) {
            // アタック処理
            attackToEnemy(damage, attackers);
        }
        
        if (healing > 0) {
            // 回復処理
            healMember(healing);
        }
        
        // 敵にダメージを与えた後の処理を設定
        CallFunc* func;
        if (afterHp <= 0) {
            func = CallFunc::create(CC_CALLBACK_0(GameLayer::winAnimation, this));      // 敵のHPが0以下の場合、Winアニメーションを表示
        }else{
            func = CallFunc::create(CC_CALLBACK_0(GameLayer::attackFromEnemy, this));
        }
        
        runAction(Sequence::create(DelayTime::create(0.5), func, nullptr));
    }
}

// 3個以上並んだボールの存在チェック
bool GameLayer::existsLinedBalls() {
    // ボールのパラメータを初期化する
    initBallParams();
    
    // 消去される順番の初期化
    _maxRemovedNo = 0;
    
    // 全ボールに対して、それぞれのボールの並びをチェックする
    for (int x = 1; x <= BALL_NUM_X; x++) {
        for (int y = 1; y <= BALL_NUM_Y; y++) {
            // x方向にボールをチェック
            checkedBall(BallSprite::PositionIndex(x, y), Direction::x);
            // y方向にボールをチェック
            checkedBall(BallSprite::PositionIndex(x, y), Direction::y);
        }
    }
    
    // 戻り値の決定
    return _maxRemovedNo > 0;       // ボールが並んでいたら、「true」が返る
}

// 全てのボールのBallTypeを取得
Map<int, BallSprite*> GameLayer::getAllBalls() {
    auto balls = Map<int, BallSprite*> ();
    
    // GameLayerが持つ全ノードから、BallSpriteクラスのノードを抽出する
    for (auto object : getChildren()) {
        auto ball = dynamic_cast<BallSprite*>(object);
        if (ball) {
            balls.insert(ball->getTag(), ball);
        }
    }
    
    return balls;
}

// 指定方向のボールと同じ色かチェックする
bool GameLayer::isSameBallType(BallSprite::PositionIndex current, Direction direction) {
    // 全てのボールのBallTypeを取得
    auto allBalls = getAllBalls();
    
    // 対象のボールがない場合は、「false」を返す
    if (direction == Direction::x) {
        if (current.x + 1 > BALL_NUM_X) {
            // 列が存在しない場合は抜ける
            return false;
        }
    }else{
        if (current.y + 1 > BALL_NUM_Y) {
            // 行が存在しない場合は抜ける
            return false;
        }
    }
    
    // 現在のボールを取得
    int currentTag = BallSprite::generateTag(BallSprite::PositionIndex(current.x, current.y));  // 位置インデックスから現在の位置のボールを取得する
    BallSprite* currentBall = allBalls.at(currentTag);
    
    // 次のボールを取得
    int nextTag;        // 隣の位置インデックスからタグを取得する
    if (direction == Direction::x) {
        nextTag = BallSprite::generateTag(BallSprite::PositionIndex(current.x + 1, current.y));
    }else{
        nextTag = BallSprite::generateTag(BallSprite::PositionIndex(current.x, current.y + 1));
    }
    
    // 隣のボールを取得する
    auto nextBall = allBalls.at(nextTag);
    
    // 2つのボールの種類を比較する
    if (currentBall->getBallType() == nextBall->getBallType()) {
        // 次のボールが同じBallTypeである
        return true;
    }
    
    return false;
}

// ボールのパラメータを初期化する
void GameLayer::initBallParams() {
    // 全てのボールのBallTypeを取得
    auto allBalls = getAllBalls();
    
    for (auto ball : allBalls) {
        ball.second->resetParams();     // 全ボールのパラメータをリセットする
    }
}

// 全ボールに対してボールの並びをチェックする
void GameLayer::checkedBall(BallSprite::PositionIndex current, Direction direction) {
    // 全てのボールのBallTypeを取得
    auto allBalls = getAllBalls();
    
    // 検索するタグの生成
    int tag = BallSprite::generateTag(BallSprite::PositionIndex(current.x, current.y));
    BallSprite* ball = allBalls.at(tag);
    
    // 指定方法のチェック済みフラグを取得
    bool checked;
    if (direction == Direction::x) {
        checked = ball->getCheckedX();
    }else{
        checked = ball->getCheckedY();
    }
    
    if (!checked) { // 未チェックのボールのみチェックする
        int num = 0;
        
        while (true) {  // ボールが並んでいたら、「true」が返る
            // 検索位置を取得
            BallSprite::PositionIndex searchPosition;
            if (direction == Direction::x) {
                searchPosition = BallSprite::PositionIndex(current.x + num, current.y);
            }else{
                searchPosition = BallSprite::PositionIndex(current.x, current.y + num);
            }
            
            // 次のボールと同じballTypeかチェックする
            if (isSameBallType(searchPosition, direction)) {
                // 次のボールと同じballType
                int nextTag = BallSprite::generateTag(searchPosition);
                auto nextBall = allBalls.at(nextTag);
                
                // チェックしたボールのチェック済みフラグを立てる
                if (direction == Direction::x) {
                    nextBall->setCheckedX(true);                    // チェックを行ったので、フラグを「true」とする
                }else{
                    nextBall->setCheckedY(true);                    // チェックを行ったので、フラグを「true」とする
                }
                
                num++;
            }else{
                // 次のボールが異なるballType
                
                if (num >= 2) {                                     // numが2以上ということは、3個以上ボールが並んでいる状態
                    int removedNo = 0;                              // removedNoは、アニメーション時においてボールの消える順を表す
                    
                    // 消去するボールカウント
                    if (_removeNumbers.size() <= _chainNumber) {
                        // 配列が存在しない場合は追加する
                        std::map<BallSprite::BallType, int> removeNumber;
                        _removeNumbers.push_back(removeNumber);
                    }
                    _removeNumbers[_chainNumber][ball->getBallType()] += num + 1;       // 消したボールの個数を記録する
                    
                    // すでにRemovedNoがあるものが存在するかチェック
                    for (int i = 0; i <= num; i++) {
                        BallSprite::PositionIndex linedPosition;
                        if (direction == Direction::x) {
                            linedPosition = BallSprite::PositionIndex(current.x + i, current.y);
                        }else{
                            linedPosition = BallSprite::PositionIndex(current.x, current.y + i);
                        }
                        
                        int linedBallTag = BallSprite::generateTag(linedPosition);
                        auto linedBall = allBalls.at(linedBallTag);
                        
                        if (linedBall->getRemovedNo() > 0) {
                            // すでにRemovedNoがあるものが存在するので、removedNoを取得し次の処理を行う
                            removedNo = linedBall->getRemovedNo();      // 縦・横の両方が揃っているボールは、同じタイミングで消えるようにする
                            break;
                        }
                    }
                    
                    // 消去する順番のカウントアップ
                    if (removedNo == 0) {
                        removedNo = ++_maxRemovedNo;
                    }
                    
                    // 3個以上並んでいた場合は、removedNoをセットする
                    for (int i = 0; i <= num; i++) {
                        BallSprite::PositionIndex linedPosition;
                        if (direction == Direction::x) {
                            linedPosition = BallSprite::PositionIndex(current.x + i, current.y);
                        }else{
                            linedPosition = BallSprite::PositionIndex(current.x, current.y + i);
                        }
                        
                        int linedBallTag = BallSprite::generateTag(linedPosition);
                        auto linedBall = allBalls.at(linedBallTag);
                        linedBall->setRemovedNo(removedNo);             // ボールが消える順番をセットする
                    }
                }
                
                break;
            }
        };
        
        // 指定方向をチェック済みとする
        if (direction == Direction::x) {
            ball->setCheckedX(true);            // 揃っていなくても、チェックが終わったので、「true」とする
        }else{
            ball->setCheckedY(true);            // 揃っていなくても、チェックが終わったので、「true」とする
        }
    }
}

// ボールの削除とボールの生成
void GameLayer::removeAndGenerateBalls() {
    // 全てのボールのBallTypeを取得
    auto allBalls = getAllBalls();
    
    int maxRemovedNo = 0;
    
    for (int x = 1; x <= BALL_NUM_X; x++) {
        int fallCount = 0;          // ボールが落下する段数をカウントする
        
        for (int y = 1; y <= BALL_NUM_Y; y++) {
            int tag = BallSprite::generateTag(BallSprite::PositionIndex(x, y));
            
            auto ball = allBalls.at(tag);
            
            if (ball) {
                int removedNoForBall = ball->getRemovedNo();
                
                if (removedNoForBall > 0) {
                    // 落ちる段数をカウント
                    fallCount++;            // ボールが1つ消えるため、それより上にあるボールはプラス1段落下することになる
                    
                    if (removedNoForBall > maxRemovedNo) {
                        maxRemovedNo = removedNoForBall;
                    }
                }else{
                    // 落ちる段数をセット
                    ball->setFallCount(fallCount);      // 消えないボールに対して、落下段数をセットする
                }
            }
        }
        
        // ボールを生成する
        generateBalls(x, fallCount);
    }
    
    // ボールの消去&落下アニメーション
    animationBalls();
}

// ボールを生成する
void GameLayer::generateBalls(int xLineNum, int fallCount) {
    for (int i = 1; i <= fallCount; i++) {
        // ボールを生成する
        auto positionIndex = BallSprite::PositionIndex(xLineNum, BALL_NUM_Y + i);
        auto ball = newBalls(positionIndex, false);                                     // 落下するボールは、表示させない（アニメーション中に表示する）
        ball->setFallCount(fallCount);
    }
}

// ボールの消去と落下アニメーション
void GameLayer::animationBalls() {
    // 全てのボールのBallTypeを取得
    auto allBalls = getAllBalls();
    
    for (auto ball : allBalls) {
        // ボールのアニメーションを実行する
        ball.second->removingAndFallingAnimation(_maxRemovedNo);
    }
}

void GameLayer::initEnemy() {
    // 敵の情報
    _enemyData = Character::create();
    _enemyData->retain();
    _enemyData->setMaxHp(10000 * _level);                                       // レベルよって、HPを変化
    _enemyData->setHp(10000 * _level);
    _enemyData->setElement(Character::Element::Wind);
    _enemyData->setTurnCount(3);
    
    // 敵の表示
    _enemy = Sprite::create(StringUtils::format("Enemy%d.png", _level));        // レベルによって、敵のグラフィックを変更
    _enemy->setPosition(Point(320, 660 + (WINSIZE.height - 660) / 2));
    addChild(_enemy, ZOrder::Enemy);
    
    // 敵のヒットポイントバー枠の表示
    auto hpBg = Sprite::create("HpEnemyBackground.png");
    hpBg->setPosition(Point(320, 530 + (WINSIZE.height - 660) / 2));
    addChild(hpBg, ZOrder::EnemyHp);
    
    // 敵のヒットポイントバーの表示
    _hpBarForEnemy = ProgressTimer::create(Sprite::create("HpEnemyRed.png"));
    _hpBarForEnemy->setPosition(Point(hpBg->getContentSize().width / 2, hpBg->getContentSize().height / 2));
    _hpBarForEnemy->setType(ProgressTimer::Type::BAR);
    _hpBarForEnemy->setMidpoint(Point::ZERO);
    _hpBarForEnemy->setBarChangeRate(Point(1, 0));
    _hpBarForEnemy->setPercentage(_enemyData->getHpPercentage());
    hpBg->addChild(_hpBarForEnemy);
}

void GameLayer::initMembers() {
    std::vector<std::string> fileNames {
        "CardBlue.png",
        "CardRed.png",
        "CardGreen.png",
        "CardYellow.png",
        "CardPurple.png"
    };
    
    std::vector<Character::Element> elements {
        Character::Element::Water,
        Character::Element::Fire,
        Character::Element::Wind,
        Character::Element::Holy,
        Character::Element::Shadow,
    };
    
    // 味方のメンバー数ループする
    for (int i = 0; i < fileNames.size(); i++) {
        //メンバー
        auto memberData = Character::create();
        memberData->setMaxHp(200);
        memberData->setHp(200);
        memberData->setElement(elements[i]);
        _memberDatum.pushBack(memberData);
        
        // メンバーの表示
        auto member = Sprite::create(fileNames[i].c_str());
        member->setPosition(Point(70 + i * 125, 598));
        addChild(member, ZOrder::Char);
        
        // メンバーヒットポイントバー枠の表示
        auto hpBg = Sprite::create("HpCardBackground.png");
        hpBg->setPosition(Point(70 + i * 125, 554));
        addChild(hpBg, ZOrder::CharHp);
        
        // メンバーヒットポイントバーの表示
        auto hpBarForMember = ProgressTimer::create(Sprite::create("HpCardGreen.png"));
        hpBarForMember->setPosition(Point(hpBg->getContentSize().width / 2, hpBg->getContentSize().height / 2));
        hpBarForMember->setType(ProgressTimer::Type::BAR);
        hpBarForMember->setMidpoint(Point::ZERO);
        hpBarForMember->setBarChangeRate(Point(1, 0));
        hpBarForMember->setPercentage(memberData->getHpPercentage());
        hpBg->addChild(hpBarForMember);
        
        // 配列に格納
        // 味方のメンバー配列で用意しているので、ここで追加する
        _members.pushBack(member);
        _hpBarForMemebers.pushBack(hpBarForMember);
    }
}

// ダメージ計算
void GameLayer::calculateDamage(int &chainNum, int &healing, int &damage, std::set<int> &attackers) {
    auto removeIt = _removeNumbers.begin();
    while (removeIt != _removeNumbers.end()) {
        auto ballIt = (*removeIt).begin();
        while (ballIt != (*removeIt).end()) {
            if ((*ballIt).first == BallSprite::BallType::Pink) {
                // 回復
                healing += 5;
            }else{
                // アタッカー分のデータを繰り返す
                for (int i = 0; i < _memberDatum.size(); i++) {
                    // メンバー情報
                    auto memberData = _memberDatum.at(i);
                    
                    // メンバーのHPが0の場合は、以下の処理を行わない
                    if (memberData->getHp() <= 0) {
                        continue;
                    }
                    
                    // 消されたボールとアタッカーの属性よりアタッカーの判定
                    if (isAttackers((*ballIt).first, memberData->getElement())) {
                        // アタッカー情報の保存
                        attackers.insert(i);
                        
                        // ダメージ
                        damage += Character::getDamage((*ballIt).second, chainNum, memberData, _enemyData);
                    }
                }
            }
            
            chainNum++;
            ballIt++;
        }
        
        removeIt++;
    }
}

// アタッカー判定
bool GameLayer::isAttackers(BallSprite::BallType type, Character::Element element) {
    switch (type) {
        case BallSprite::BallType::Red:
            // 赤ボール:火属性
            if (element == Character::Element::Fire) {
                return true;
            }
            break;
            
        case BallSprite::BallType::Blue:
            // 青ボール:水属性
            if (element == Character::Element::Water) {
                return true;
            }
            break;
            
        case BallSprite::BallType::Green:
            // 緑ボール:風属性
            if (element == Character::Element::Wind) {
                return true;
            }
            break;
            
        case BallSprite::BallType::Yellow:
            // 黄ボール:光属性
            if (element == Character::Element::Holy) {
                return true;
            }
            break;
            
        case BallSprite::BallType::Purple:
            // 紫ボール:闇属性
            if (element == Character::Element::Shadow) {
                return true;
            }
            break;
            
        default:
            break;
    }
    
    return false;
}

// 敵への攻撃
void GameLayer::attackToEnemy(int damage, std::set<int> attackers) {
    // 敵のHPを取得する
    float preHpPercentage = _enemyData->getHpPercentage();
    
    // ダメージをセットする
    int afterHp = _enemyData->getHp() - damage;
    if (afterHp < 0) {
        afterHp = 0;
    }
    _enemyData->setHp(afterHp);                     // 敵のHPをセットする
    
    // 敵のヒットポイントバーのアニメーション
    auto act = ProgressFromTo::create(0.5, preHpPercentage, _enemyData->getHpPercentage());     // アニメーションを用意する
    _hpBarForEnemy->runAction(act);
    
    // 敵の被ダメージアニメーション
    _enemy->runAction(vibratingAnimation(afterHp));                                             // アニメーションを用意する
    
    // メンバーの攻撃アニメーション
    for (auto attacker : attackers) {
        auto member = _members.at(attacker);
        member->runAction(Sequence::create(MoveBy::create(0.1, Point(0, 10)),
                                           MoveBy::create(0.1, Point(0, -10)), nullptr));       // アニメーションの用意する
    }
}

// メンバーの回復
void GameLayer::healMember(int healing) {
    for (int i = 0; i < _memberDatum.size(); i++) {
        // メンバーデータ取得
        auto memberData = _memberDatum.at(i);
        
        // HPが0の場合は、回復しない
        if (memberData->getHp() <= 0) {
            continue;
        }
        
        // メンバーを回復する
        float preHpPercentage = memberData->getHpPercentage();
        int afterHp = memberData->getHp() + healing;
        if (afterHp > memberData->getMaxHp()) {
            afterHp = memberData->getMaxHp();
        }
        memberData->setHp(afterHp);                                                                 // 味方のHPをセットする
        
        // メンバーHPアニメーション
        auto act = ProgressFromTo::create(0.5, preHpPercentage, memberData->getHpPercentage());     // アニメーションを用意する
        _hpBarForMemebers.at(i)->runAction(act);
    }
}

// 敵からの攻撃
void GameLayer::attackFromEnemy() {
    if (!_enemyData->isAttackTurn()) {
        // 敵の攻撃ターンでない場合は、一連の処理を終わらせる
        endAnimation();
        return;
    }
    
    // メンバーを1人選択
    int index;
    Character* memberData;
    
    do {
        // ランダムでメンバーを選択
        index = _distForMember(_engine);
        memberData = _memberDatum.at(index);
        
        // HPが0のメンバーを選択した場合は、再度選択し直す
    } while (memberData->getHp() <= 0);
    
    auto member = _members.at(index);
    auto hpBarForMember = _hpBarForMemebers.at(index);
    
    // メンバーにダメージを与える
    float preHpPercentage = memberData->getHpPercentage();
    int afterHp = memberData->getHp() - 25;
    if (afterHp > memberData->getMaxHp()) {
        afterHp = memberData->getMaxHp();
    }
    memberData->setHp(afterHp);
    
    // メンバーヒットポイントバーのアニメーション
    auto act = ProgressFromTo::create(0.5, preHpPercentage, memberData->getHpPercentage());     // アニメーションを用意する
    hpBarForMember->runAction(act);
    
    // メンバーの被ダメージアニメーション
    member->runAction(vibratingAnimation(afterHp));                                             // アニメーションを用意する
    
    // 敵の攻撃アニメーション
    auto seq = Sequence::create(MoveBy::create(0.1, Point(0, -10)),
                                MoveBy::create(0.1, Point(0, -10)), NULL);                      // アニメーションを用意する
    _enemy->runAction(seq);
    
    // 味方の全員をチェック
    bool allHpZero = true;
    
    for (auto character : _memberDatum) {
        if (character->getHp() > 0) {
            allHpZero = false;
            break;
        }
    }
    
    // アニメーション終了時処理
    CallFunc* func;
    if (!allHpZero) {
        func = CallFunc::create(CC_CALLBACK_0(GameLayer::loseAnimation, this));                 // メンバーのHPが0以下の場合、loseアニメーションを表示
    }else{
        func = CallFunc::create(CC_CALLBACK_0(GameLayer::endAnimation, this));
    }
    runAction(Sequence::create(DelayTime::create(0.5), func, nullptr));
}

// アニメーション終了時処理
void GameLayer::endAnimation() {
    // タップを有効にする
    _touchable = true;
}

// 振動アニメーション
Spawn* GameLayer::vibratingAnimation(int afterHp) {
    // 振動アニメーション
    auto move = Sequence::create(MoveBy::create(0.025, Point( 5,  5)),
                                 MoveBy::create(0.025, Point(-5, -5)),
                                 MoveBy::create(0.025, Point(-5, -5)),
                                 MoveBy::create(0.025, Point( 5,  5)),
                                 MoveBy::create(0.025, Point( 5, -5)),
                                 MoveBy::create(0.025, Point(-5,  5)),
                                 MoveBy::create(0.025, Point(-5,  5)),
                                 MoveBy::create(0.025, Point( 5, -5)),
                                 nullptr);
    
    // ダメージ時に色を赤くする
    Action* tint;
    if (afterHp > 0) {
        // HPが0より大きい場合は、元の色に戻す
        tint = Sequence::create(TintTo::create(0, 255, 0, 0),
                                DelayTime::create(0.2),
                                TintTo::create(0, 255, 255, 255),
                                nullptr);
    }else{
        // HPが0の場合は、赤いままにする
        tint = TintTo::create(0, 255, 0, 0);
    }
    
    return Spawn::create(move, tint, nullptr);
}

/** レベル表示レイヤーの表示 */
void GameLayer::initLevelLayer() {
    // レベルレイヤーの生成
    auto levelLayer = LayerColor::create(Color4B(0, 0, 0, 191), WINSIZE.width, WINSIZE.height);    // 黒い透過の背景を表示
    levelLayer->setPosition(Point::ZERO);
    levelLayer->setTag(TAG_LEVEL_LAYER);
    addChild(levelLayer, ZOrder::Level);
    
    // レベルの表示
    auto levelSprite = Sprite::create("Level.png");
    levelSprite->setPosition(Point(WINSIZE.width * 0.45, WINSIZE.height * 0.5));
    levelLayer->addChild(levelSprite);
    
    // レベル数の表示
    auto levelNumPath = StringUtils::format("%d.png", _level);                                      // StringUtilsクラスformat関数でフォーマットが簡単に利用できる
    auto levelNumSprite = Sprite::create(levelNumPath.c_str());
    levelNumSprite->setPosition(Point(WINSIZE.width * 0.85, WINSIZE.height * 0.5));
    levelLayer->addChild(levelNumSprite);
    
    // 1.5秒後に消えるようにする
    scheduleOnce(schedule_selector(GameLayer::removeLevelLayer), 1.5);
}

/** レベル表示レイヤーの削除 */
void GameLayer::removeLevelLayer(float dt) {
    // タップ可能とする
    _touchable = true;
    
    // 0.5秒で消えるようにする
    auto levelLayer = getChildByTag(TAG_LEVEL_LAYER);
    levelLayer->runAction(Sequence::create(FadeTo::create(0.5, 0),
                                           RemoveSelf::create(),nullptr));
}

/** Winアニメーション*/
void GameLayer::winAnimation() {
    // 白い背景を用意する
    auto whiteLayer = LayerColor::create(Color4B(255, 255, 255, 127), WINSIZE.width, WINSIZE.height);       // 白い透過の背景を表示する
    whiteLayer->setPosition(Point::ZERO);
    addChild(whiteLayer, ZOrder::Result);
    
    // Winグラフィックを表示する
    auto win = Sprite::create("Win.png");
    win->setPosition(Point(WINSIZE.width / 2, WINSIZE.height / 2));
    addChild(win, ZOrder::Result);
    
    // 次のレベルを設定（Level13の次はないので、Level11に戻る）
    if (_level >= 3) {
        _nextlevel = 1;
    }else{
        _nextlevel = _level + 1;                // 次のレベルを指定する
    }
    
    // 指定秒数に次のシーンへ
    scheduleOnce(schedule_selector(GameLayer::nextScene), 3);
}

/** Loseアニメーション */
void GameLayer::loseAnimation() {
    // 黒い背景を用意する
    auto blackLayer = LayerColor::create(Color4B(0, 0, 0, 127), WINSIZE.width, WINSIZE.height);         // 黒い透過の背景を表示する
    blackLayer->setPosition(Point::ZERO);
    addChild(blackLayer, ZOrder::Result);
    
    // Loseグラフィックを表示する
    auto lose = Sprite::create("Lose.png");
    lose->setPosition(Point(WINSIZE.width / 2, WINSIZE.height / 2));
    addChild(lose, ZOrder::Result);
    
    // 次のレベルを設定
    _nextlevel = 1;
    
    // 指定秒数に次のシーンへ
    scheduleOnce(schedule_selector(GameLayer::nextScene), 3);
}

// 次のシーンへ遷移
void GameLayer::nextScene(float dt) {
    // 次のシーンを生成する
    auto scene = GameLayer::createScene(_nextlevel);
    Director::getInstance()->replaceScene(scene);
}