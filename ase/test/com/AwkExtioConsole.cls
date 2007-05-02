VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "AwkExtioConsole"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Private m_eof As Boolean

Private Sub Class_Initialize()
    m_eof = False
    'MsgBox "AwkExtio Initializeing"
End Sub

Private Sub Class_Terminate()
    'MsgBox "AwkExtio Terminating..."
End Sub

Public Property Let EOF(v As Boolean)
    m_eof = v
End Property

Public Property Get EOF() As Boolean
     EOF = m_eof
End Property

