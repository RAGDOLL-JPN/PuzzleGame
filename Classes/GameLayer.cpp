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

USING_NS_CC;

// コンストラクタ
GameLayer::GameLayer()
: _movingBall(nullptr)
, _movedBall(false)
, _touchable(true)
, _maxRemovedNo(0)
, _chainNumber(0) {
    // 乱数初期化および各ボールの出現の重みを指定
    std::random_device device;
    _engine = std::default_random_engine(device());     // ボールの重みを設定
    _distForBall = std::discrete_distribution<int> {20, 20, 20, 20, 20, 10};
}

// シーンの生成
Scene* GameLayer::createScene() {
    auto scene = Scene::create();
    auto layer = GameLayer::create();
    scene->addChild(layer);
    
    return scene;
}

// 初期化
bool GameLayer::init() {
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
    
    initBackground();           // 背景の初期化
    initBalls();                // ボールの初期表示
    
    return true;
}

// 背景の初期化
void GameLayer::initBackground() {
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
        // タップを有効にする
        _touchable = true;
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
    
    if (!checked) {
        int num = 0;
        
        while (true) {
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
                    nextBall->setCheckedY(true);
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
                    _removeNumbers[_chainNumber][ball->getBallType()] += num + 1;
                    
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
                            linedPosition = BallSprite::PositionIndex(current.x + 1, current.y);
                        }else{
                            linedPosition = BallSprite::PositionIndex(current.x, current.y + 1);
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