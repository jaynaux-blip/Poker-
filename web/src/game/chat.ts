import { Rng } from '../core/rng';

/** Table chat: dealer lines plus the things online players actually type at 3 a.m. */

export interface ChatLine {
  who: string; // '' for dealer/system
  text: string;
  kind: 'dealer' | 'player' | 'system' | 'rival' | 'hero';
  time: number;
}

const WIN_BRAG = ['ship it', 'ty', 'ty ty', 'thx for the chips', 'lets gooo', 'knew it', 'EZ', 'gg wp'];
const LOSE_SALT = ['unreal', 'every. single. time.', 'rigged site lol', 'how do you call that', 'wow', 'nice river', 'sick', 'of course', 'this site man'];
const NICE = ['nh', 'nh sir', 'gh', 'wp'];
const BUST = ['gg', 'gg all', 'gl everyone', 'gg, bed time', 'and im out. gl'];
const IDLE = [
  'anyone else up at 3am lol', 'gl all', 'blinds are flying', 'coffee #4', 'this turbo is a lottery', 'ante up',
  'who else has work tomorrow', 'fold fold fold fold', 'card dead for an hour', 'my cat just stepped on my keyboard',
  'ICM says fold. my heart says jam', 'min cash here i come',
];

export const RIVAL = 'gh0stfold';

export function brag(rng: Rng): string {
  return rng.pick(WIN_BRAG);
}
export function salt(rng: Rng): string {
  return rng.pick(LOSE_SALT);
}
export function nice(rng: Rng): string {
  return rng.pick(NICE);
}
export function bustLine(rng: Rng): string {
  return rng.pick(BUST);
}
export function idle(rng: Rng): string {
  return rng.pick(IDLE);
}

export const RIVAL_LINES = {
  arrive: ['evening, table.', 'moved again. fine.', 'hi fish. just kidding. mostly.'],
  winVsHero: ['thanks for the chips', 'you had it. i had more.', 'predictable.', 'called it before the flop'],
  loseVsHero: ['nice hand. won\'t happen twice', 'enjoy that one', 'ok. noted.'],
  heroBust: ['gg. see you tomorrow night', 'gg. go get some sleep', 'back to the freerolls'],
  bustsSelf: ['gg. you got lucky tonight', 'well played. this time.'],
};
