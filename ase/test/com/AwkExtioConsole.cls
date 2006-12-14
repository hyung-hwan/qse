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
Private m_active As Boolean
Private m_count As Long

Public Enum Status
    STATUS_ON = 1
    STATUS_OFF = 2
End Enum

Private Sub Class_Initialize()
    m_active = False
    m_count = 0
    MsgBox "AwkExtio Initializeing"
End Sub

Private Sub Class_Terminate()
    MsgBox "AwkExtio Terminating..."
End Sub

Public Property Let Active(v As Boolean)
    m_active = v
End Property

Public Property Get Active() As Boolean
     Active = m_active
End Property

Public Property Let Count(v As Long)
    m_count = v
End Property

Public Property Get Count() As Long
    Count = m_count
End Property
