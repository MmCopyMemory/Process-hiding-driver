# Process hiding driver

A Windows driver to hide processes by unlinking them from the ActiveProcessLinks list.<br>

---

## Features

- Hide any process with its Process ID
- Show any process with its Process ID

---

## How it works

Driver gets the processes EPROCESS handle and then adds OFFSET_ActiveProcessLinks to cast it to LIST_ENTRY.<br>
It "deletes" or empties the list entry to hide the process and on restoration it uses the stored original entry to return it back to existence.<br>
