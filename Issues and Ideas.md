#ToDo
- Make script for power meter
- Debug fmnCalculator to M stuck at 4092
- Set up O'scope for Python
- Re-evaluate bitMask() function (it's just adding complexity)



--------------
: ──────────────────────────────────────────────────────────────────

--------------
11/13/25
# NEXT ISSUE
### Problem Name
: Problem description


--------------
: ──────────────────────────────────────────────────────────────────

--------------
11/10/25
# Optimization Idea
: Reducing the number of register programming cycles to the minimum possible

### Identify all values of M that result in 'closest approach' to zero error
In the for-M-loop in fmn2freq it may be that the magnitude of the lowest error is the same for different values of M. Grouping all frequencies into fewer M's could mean less programming of register 1. In this case it would be necessary to catalog all values of M that matched the lowest error value found. I don't want a 'close' match but a well defined 'less than or equal to' value.

### Change freq2FMN to store a dictionary of all the values of M that match the minimum error
*** Pseudo Code ***

    - M_list = []
    - If the current_error is less than the last_error:
        1) error = round((error * 1000), 3)    # Limit error to kHz accuracy
        2) Empty the M_dictionary
        3) Add the new M to the M_dictionary
    - Else if current_error is equal to the last_error:
        1) Add the new M to the M_dictionary
    - M_dictionary = {error:M_list}
    - Return M_dictionary

Integrate the above with current freq2FMN and replace the quick-exit code with the above dictionary builder?

At the next code level up each M_dictionary will be added to the dictionary-of-dictionaries called freq_M_targets[].
That will look like:
: freq_M_targets = {freq:M_dictionary}

--------------
: ──────────────────────────────────────────────────────────────────

--------------
11/8/25
# Interesting Idea

### AI said
: You brute-force M from 4095→2 and compute an error each time. On an 8-bit MCU that’s expensive.

### AI suggested
: A faster and exact approach is a bounded continued-fraction solver / best-rational approximation of floatFrac with a bounded denominator M ≤ 4095. That finds the optimal (F, M) in microseconds vs. scanning thousands of candidates.
