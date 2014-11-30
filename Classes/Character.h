//
//  Character.h
//  PuzzleGame
//
//  Created by NAKASHIMAshinichiro on 2014/11/30.
//
//

#ifndef PuzzleGame_Character_h
#define PuzzleGame_Character_h

#include "cocos2d.h"

class Character : public cocos2d::Ref {
public:
    // キャラクター属性
    enum class Element {// キャラクター属性の列挙体
        Fire,           // 火属性
        Water,          // 水属性
        Wind,           // 風属性
        Holy,           // 光属性
        Shadow,         // 闇属性
        None,           // 無属性
    };
    
protected:
    int _remainingTurn;                         // 攻撃するまでの残りターン
    
    // マクロによりアクセッサを定義する
    CC_SYNTHESIZE(int, _hp, Hp);                // ヒットポイント
    CC_SYNTHESIZE(int, _maxHp, MaxHp);          // 最大ヒットポイント
    CC_SYNTHESIZE(int, _attack, Attack);        // 攻撃力
    CC_SYNTHESIZE(Element, _element, Element);  // 属性
    CC_PROPERTY(int, _turnCount, TurnCount);    // 攻撃ターン数（敵の場合）
    
public:
    Character();                                                        // コンストラクタ
    static Character* create();                                         // インスタンス生成 / init関数がないため、マクロを利用せずにcreate関数を定義する
    
    float getHpPercentage();                                            // ヒットポイント(%値)取得
    bool isAttackTurn();                                                // 攻撃ターン（敵の場合）チェック
    static int getDamage(int ballCount, int chainCount,
                         Character* attackker, Character* defender);    // ダメージ数取得

protected:
    static float getElementBonus(Element attackElement,
                                 Element defensElement);                // 属性による攻撃ボーナス
};

#endif