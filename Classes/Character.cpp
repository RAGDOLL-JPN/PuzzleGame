//
//  Character.cpp
//  PuzzleGame
//
//  Created by NAKASHIMAshinichiro on 2014/11/30.
//
//

#include "Character.h"

USING_NS_CC;

// コンストラクタ
Character::Character()
// 変数の初期化
: _hp(0)
, _maxHp(0)
, _attack(0)
, _element(Element::None)
, _turnCount(0)
, _remainingTurn(0)
{
    
}

// インスタンス生成
Character* Character::create() {
    Character *pRet = new Character();
    pRet->autorelease();
    
    return pRet;
}

// 攻撃ターン数取得
int Character::getTurnCount() {
    return _turnCount;
}

// 攻撃ターン数設定
void Character::setTurnCount(int turnCount) {
    _turnCount = turnCount;
    _remainingTurn = _turnCount;
}

// ヒットポイント(%値)取得
float Character::getHpPercentage() {
    return _hp * 100.f / _maxHp;
}

// 攻撃ターン（敵の場合）チェック
bool Character::isAttackTurn() {
    // 残りターンの計算
    _remainingTurn--;                   // 攻撃までのターンを減算
    
    if (_remainingTurn <= 0) {
        // 残りターン数が「0」の場合、攻撃を行う
        _remainingTurn = _turnCount;    // 攻撃を行うので、残りターンをリセットする
        return true;
    }
    
    return false;
}

// ダメージ数取得
int Character::getDamage(int ballCount, int chainCount, Character* attacker, Character* defender) {
    // 消したボールの個数によるダメージ
    float baseDamage = ballCount / 3.0 * 100;                                               // 基本ダメージの計算
    
    // 連鎖ボーナス
    float chainBonus = powf(1.1, chainCount - 1);                                           // 攻撃ボーナスの計算
    
    // 属性ボーナス
    float elementBonus = getElementBonus(attacker->getElement(), defender->getElement());   // 攻撃ボーナスの計算
    
    // ダメージにボーナスを乗算したものを返す
    return baseDamage * chainBonus * elementBonus;
}

// 属性による攻撃ボーナス
float Character::getElementBonus(Element attackElement, Element defenseElement) {
    // 攻撃属性
    switch (attackElement) {
        case Element::Fire: {
            // 防御属性
            switch (defenseElement) {
                    // 攻撃属性・防御属性により、得手不得手を設定する
                case Element::Wind:
                    return 2;
                case Element::Water:
                    return 0.5;
                default:
                    return 1;
            }
            break;
        }
        case Element::Water: {
            // 防御属性
            switch (defenseElement) {
                case Element::Fire:
                    return 2;
                case Element::Wind:
                    return 0.5;
                default:
                    return 1;
            }
            break;
        }
        case Element::Wind: {
            // 防御属性
            switch (defenseElement) {
                case Element::Water:
                    return 2;
                case Element::Fire:
                    return 0.5;
                default:
                    return 1;
            }
            break;
        }
        case Element::Holy: {
            // 防御属性
            switch (defenseElement) {
                case Element::Shadow:
                    return 2;
                default:
                    return 1;
            }
            break;
        }
        case Element::Shadow: {
            // 防御属性
            switch (defenseElement) {
                case Element::Holy:
                    return 2;
                default:
                    return 1;
            }
            break;
        }
        default: {
            return 1;
        }
    }
}