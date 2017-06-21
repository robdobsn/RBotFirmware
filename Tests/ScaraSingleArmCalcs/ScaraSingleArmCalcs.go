package main

// Only the plain math package is needed for the formulas.	
import (
	"fmt"
	"math"
)

// The lengths of the two segments of the robot’s arm. Using the same length for both segments allows the robot to reach the (0,0) coordinate.	
const (
	len1 = 100.0
	len2 = 100.0
)

// The law of cosines, transfomred so that C is the unknown. The names of the sides and angles correspond to the standard names in mathematical writing. Later, we have to map the sides and angles from our scenario to a, b, c, and C, respectively.	
func lawOfCosines(a, b, c float64) (C float64) {
//	fmt.Printf("Calculating acos %v, a %v, b %v, c %v", (a*a + b*b - c*c) / (2 * a * b), a, b, c)
	return math.Acos((a*a + b*b - c*c) / (2 * a * b))
}

// The distance from (0,0) to (x,y). HT to Pythagoras.	
func distance(x, y float64) float64 {
	return math.Sqrt(x*x + y*y)
}

// Calculating the two joint angles for given x and y.	
func angles(x, y float64) (A1, A2 float64) {
	//First, get the length of line dist.	
	dist := distance(x, y)
	// Calculating angle D1 is trivial. Atan2 is a modified arctan() function that returns unambiguous results.	
	D1 := math.Atan2(y, x)
	// D2 can be calculated using the law of cosines where a = dist, b = len1, and c = len2.	
	D2 := lawOfCosines(dist, len1, len2)
	// Then A1 is simply the sum of D1 and D2.	
	A1 = D1 + D2
	// A2 can also be calculated with the law of cosine, but this time with a = len1, b = len2, and c = dist.	
	A2 = lawOfCosines(len1, len2, dist)

	fmt.Printf("dist %v, D1 %v, D2 %v, A1 %v, A2 %v", dist, D1, D2, A1, A2)

	return A1, A2
}

// Convert radians into degrees.	
func deg(rad float64) float64 {
	return rad * 180 / math.Pi
}

func convAngles(A1, A2 float64) (alpha, beta float64) {
	alpha = math.Mod(math.Pi * 2.5 - A1, math.Pi * 2)
	beta = math.Mod(math.Pi * 3 - (A2 - alpha + 2 * math.Pi), 2 * math.Pi)
	return alpha, beta
}

func show(x, y, a1, a2, alpha, beta float64) {
	fmt.Printf("x=%v, y=%v: A1=%v (%v°), A2=%v (%v°), alpha=%v (%v°), beta=%v (%v°)\n\n", 
					x, y, a1, deg(a1), a2, deg(a2), alpha, deg(alpha), beta, deg(beta))
}

func main() {

	fmt.Println("Lets do some tests. First move to (5,5):")
	x, y := 5.0, 5.0
	a1, a2 := angles(x, y)
	alpha, beta := convAngles(a1, a2)
	show (x,y,a1,a2,alpha, beta)

	fmt.Println("If y is 0 and x = Sqrt(len1^2 + len2^2), then alpha should become 45 degrees and beta should become 90 degrees.")
	x, y = math.Sqrt(len1*len1+len2*len2), 0
	a1, a2 = angles(x, y)
	alpha, beta = convAngles(a1, a2)
	show (x,y,a1,a2,alpha, beta)

	fmt.Println("Now let's try moving to (1, 190).")
	x, y = 1, 190
	a1, a2 = angles(x, y)
	alpha, beta = convAngles(a1, a2)
	show (x,y,a1,a2,alpha, beta)

	fmt.Println("n extreme case: (200,0). The arm needs to stretch along the y axis.")
	x, y = 200, 0
	a1, a2 = angles(x, y)
	alpha, beta = convAngles(a1, a2)
	show (x,y,a1,a2,alpha, beta)

	fmt.Println("And (0,200).")
	x, y = 0, 200
	a1, a2 = angles(x, y)
	alpha, beta = convAngles(a1, a2)
	show (x,y,a1,a2,alpha, beta)

	fmt.Println("Moving to (0,0) technically works if the arm segments have the same length, and if the arm does not block itself. Still the result looks a bit weird!?")
	x, y = 0, 0
	a1, a2 = angles(x, y)
	alpha, beta = convAngles(a1, a2)
	show (x,y,a1,a2,alpha, beta)

	fmt.Println("What happens if the target point is outside the reach? Like (200,200).")
	x, y = 200, 200
	a1, a2 = angles(x, y)
	alpha, beta = convAngles(a1, a2)
	show (x,y,a1,a2,alpha, beta)
}
