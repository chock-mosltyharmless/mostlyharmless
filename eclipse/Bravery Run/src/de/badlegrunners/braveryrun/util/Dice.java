package de.badlegrunners.braveryrun.util;

import java.util.Random;

/**
 * Method for rolling several different types of
 * dices.
 * 
 * TODO: Do not rely on java.util.Random, but rather make your
 * own random class so that the game behaves independent of the
 * java implementation.
 * 
 * @author chock
 */
public class Dice {
	static Random rand = null;
	
	
	/**
	 * Returns a random number between 1 and the number of sides
	 * of the dice n
	 * 
	 * @param n The number of sides of the dice
	 * @return The randomly rolled value
	 */
	public static int roll(int n) {
		if (rand == null) {
			rand = new Random();
		}		
		return rand.nextInt(n) + 1;
	}
}
