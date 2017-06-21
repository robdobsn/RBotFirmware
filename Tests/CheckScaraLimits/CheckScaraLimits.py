import math

def wrapDegrees(angle):
    return angle - 360.0 * math.floor( angle / 360.0 )

print("{:>8}{:>8}({:>6}){:>8}{:>8}{:>8}{:>8}".format("Target", "Current", "wrap", "Diff", "MinDiff", "MinTarg", "Check"));
for c in range(24):
    for t in range(24):
        current = c * 45 - 360
        target = t * 45 - 360
        wrapCurrent = wrapDegrees(current)
        wrapTarget = wrapDegrees(target)
        diff = wrapDegrees(target - current)
        minDiff = diff if diff < 180 else 360 - diff
        minTarget = target
        if wrapCurrent < 180:
            if wrapTarget < wrapCurrent + 180 and wrapTarget > wrapCurrent:
                minTarget = current + minDiff
            else:
                minTarget = current - minDiff
        else:
            if wrapTarget > wrapCurrent - 180 and wrapTarget < wrapCurrent:
                minTarget = current - minDiff
            else:
                minTarget = current + minDiff            
        checkDiff = minTarget - current if minTarget > current else current - minTarget
            

        print("{:8.2f}{:8.2f}({:6.2f}){:8.2f}{:8.2f}{:8.2f}{:8.2f}{:>8}{:>8}".format(
            target, current, wrapCurrent, diff, minDiff, minTarget, checkDiff,
            "OK" if checkDiff == minDiff else "ERROR",
            "OK" if wrapDegrees(minTarget) == wrapDegrees(target) else "ERROR"))
        
            
